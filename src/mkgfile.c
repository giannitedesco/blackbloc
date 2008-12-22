/*
* This file is part of blackbloc
* Copyright (c) 2008 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#include <blackbloc.h>
#include <gang.h>
#include <mpool.h>
#include <gfile.h>
#include <gfile-internal.h>

#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

struct manifest {
	char *m_name;
	off_t m_size;
	struct manifest *m_next;
};

struct file {
	char *f_name;
	uint32_t f_rsz;
	uint32_t f_asz;
	uint32_t f_ofs;
};

static struct mpool fmem;
static struct gang nmem;
static unsigned int num_files;
static off_t tot_sz;
static struct manifest *manifest;

static int dump_align(FILE *f, uint32_t *cur)
{
	static const uint8_t pad[GFILE_ALIGN_MASK];
	size_t sz;

	if ( (*cur & GFILE_ALIGN_MASK) == 0 )
		return 1;

	sz = GFILE_ALIGN - (*cur & GFILE_ALIGN_MASK);
	if ( fwrite(pad, sz, 1, f) != 1 || ferror(f) ) {
		fprintf(stderr, "%s: fwrite: %s\n", _func, get_err());
		return 0;
	}
	*cur += sz;
	return 1;
}

static int dump_file(FILE *f, uint32_t *cur, struct file *file)
{
	int fd;
	const uint8_t *map, *ptr;
	int ret = 0;

	fd = open(file->f_name, O_RDONLY);
	if ( fd < 0 ) 
		goto err;

	/* Why use f_asz and not f_rsz? Simple, PAGE_ALIGN always guarantees
	 * more than our maximum alignment and slack space is zero filled
	 */
	ptr = map = mmap(NULL, file->f_asz, PROT_READ, MAP_SHARED, fd, 0);
	if ( map == MAP_FAILED)
		goto err_close;

	if ( fwrite(map, file->f_asz, 1, f) == 1 )
		ret = !ferror(f);
	*cur += file->f_asz;

	munmap((void *)map, file->f_asz);
err_close:
	close(fd);
err:
	if ( !ret )
		fprintf(stderr, "%s: %s: %s\n", _func, file->f_name, get_err());
	return ret;
}

static int dump_header(FILE *f, uint32_t *cur, uint32_t num_blk)
{
	struct gfile_hdr h;
	h.h_magic = be_32(GFILE_MAGIC);
	h.h_num_blk = be_32(num_blk);
	if ( fwrite(&h, sizeof(h), 1, f) != 1 || ferror(f) ) {
		fprintf(stderr, "%s: fwrite: %s\n", _func, get_err());
		return 0;
	}
	*cur += sizeof(h);
	return 1;
}

static int dump_block(FILE *f, uint32_t *cur, uint16_t type, uint16_t id,
			uint32_t ofs, uint32_t len)
{
	struct gfile_blk b;
	b.b_type = be_16(type);
	b.b_id = be_16(id);
	b.b_ofs = be_32(ofs);
	b.b_len = be_32(len);
	b.b_flags = 0;
	if ( fwrite(&b, sizeof(b), 1, f) != 1 || ferror(f) ) {
		fprintf(stderr, "%s: fwrite: %s\n", _func, get_err());
		return 0;
	}
	*cur += sizeof(b);
	return 1;
}

static int dump_nidx(FILE *f, uint32_t *cur,
			uint32_t num_names,
			uint32_t strtab_sz)
{
	struct gfile_nidx idx;
	idx.i_num_names = be_32(num_names);
	idx.i_strtab_sz = be_32(strtab_sz);
	if ( fwrite(&idx, sizeof(idx), 1, f) != 1 || ferror(f) ) {
		fprintf(stderr, "%s: fwrite: %s\n", _func, get_err());
		return 0;
	}
	*cur += sizeof(idx);
	return 1;
}

static int dump_name(FILE *f, uint32_t *cur,
			uint32_t ofs, uint32_t len,
			uint32_t name, uint16_t nlen)
{
	struct gfile_name n;
	n.n_ofs = be_32(ofs);
	n.n_len = be_32(len);
	n.n_name = be_32(name);
	n.n_nlen = be_16(nlen);
	n.n_mode = 0;
	if ( fwrite(&n, sizeof(n), 1, f) != 1 || ferror(f) ) {
		fprintf(stderr, "%s: fwrite: %s\n", _func, get_err());
		return 0;
	}
	*cur += sizeof(n);
	return 1;
}

static int dump_str(FILE *f, uint32_t *cur, const char *str)
{
	if ( fwrite(str, strlen(str) + 1, 1, f) != 1 || ferror(f) ) {
		fprintf(stderr, "%s: fwrite: %s\n", _func, get_err());
		return 0;
	}
	*cur += strlen(str) + 1;
	return 1;
}

static int fcmp_sz(const void *_A, const void *_B)
{
	const struct file *a = _A, *b = _B;
	return b->f_rsz - a->f_rsz;
}

static int fcmp_name(const void *_A, const void *_B)
{
	const struct file *a = _A, *b = _B;
	return strcmp(a->f_name, b->f_name);
}

/* Create the gfile */
static int dump_gfile(FILE *f)
{
	struct file *f_arr;
	struct manifest *m;
	unsigned int i;
	uint64_t aligned_sz = 0;
	uint32_t strtab_sz = 0;
	uint32_t idx_sz = 0;
	uint32_t ovh_sz = 0;
	uint32_t gfl_sz = 0;
	uint32_t strtab_base = 0;
	uint32_t file_base = 0;
	uint32_t cur;
	int ret = 0;

	/* 1. Copy manifest in to file array */
	f_arr = calloc(sizeof(*f_arr), num_files);
	if ( f_arr == NULL ) {
		fprintf(stderr, "%s: calloc: %s\n", _func, get_err());
		goto err;
	}

	for(m = manifest, i = 0; m; m = m->m_next, i++) {
		f_arr[i].f_name = m->m_name;
		if ( (uint64_t)m->m_size > 0xffffffffULL ) {
			fprintf(stderr, "%s: %s: file too big\n",
				_func, m->m_name);
				goto err_free;
		}
		f_arr[i].f_rsz = m->m_size;
		f_arr[i].f_asz = ALIGN_GFILE(m->m_size);
	}

	assert(i == num_files);

	/* We're done with the manifest now */
	mpool_fini(&fmem);


	/* 2. Lay out the files and calculate on-disk sizes */

	/* Files layed out in size order and padded to record alignment.
	 * We sort by name and then fcmp_size for interesting reasons,
	 * it's needed to stabilise the sort because there may be
	 * files of the same size.
	 */
	qsort(f_arr, num_files, sizeof(*f_arr), fcmp_name);
	qsort(f_arr, num_files, sizeof(*f_arr), fcmp_sz);
	for(i = 0; i < num_files; i++) {
		f_arr[i].f_ofs = aligned_sz;
		aligned_sz += f_arr[i].f_asz;
		strtab_sz += strlen(f_arr[i].f_name) + 1;
	}

	/* maximum size of total gfile or any record is 4GB */
	if ( aligned_sz > 0xffffffffULL ) {
		fprintf(stderr, "%s: total file size exceeded\n",
			_func);
		goto err_free;
	}

	/* Calculate variables needed for layout  */
	ovh_sz = sizeof(struct gfile_hdr) +
			2 * sizeof(struct gfile_blk);
	idx_sz = sizeof(struct gfile_nidx) + 
			num_files * sizeof(struct gfile_name) +
			ALIGN_GFILE(strtab_sz);
	strtab_base = ovh_sz + sizeof(struct gfile_nidx) +
			num_files * sizeof(struct gfile_name);
	file_base = ovh_sz + idx_sz;
	gfl_sz = sizeof(struct gfile_hdr) +
			2 * sizeof(struct gfile_blk) +
			idx_sz + (uint32_t)aligned_sz;

	fprintf(stderr, " * %u KB of files aligned to %u KB\n",
			(uint32_t)tot_sz >> 10,
			(uint32_t)aligned_sz >> 10);
	fprintf(stderr, " * name_idx = %u bytes (%u KB)\n",
		idx_sz, idx_sz >> 10);
	fprintf(stderr, "   * num_names = %u\n", num_files);
	fprintf(stderr, "   * strtab_sz = %u bytes (%u KB)\n",
		strtab_sz, strtab_sz >> 10);
	fprintf(stderr, " * overhead = %lu (%lu KB)\n",
		ovh_sz + (uint32_t)aligned_sz - tot_sz,
		(ovh_sz + (uint32_t)aligned_sz - tot_sz) >> 10);
	fprintf(stderr, " * total_sz = %u (%u KB)\n",
		gfl_sz, gfl_sz >> 10);

	assert((idx_sz & GFILE_ALIGN_MASK) == 0);
	assert((strtab_base & GFILE_ALIGN_MASK) == 0);
	assert((file_base & GFILE_ALIGN_MASK) == 0);
	assert((gfl_sz & GFILE_ALIGN_MASK) == 0);

	
	/* 3. write out headers + blocks */
	cur = 0;
	if ( !dump_header(f, &cur, 2) )
		goto err_free;
	
	/* Gotta dump blocks in type/id order */
	if ( !dump_block(f, &cur, GFILE_TYPE_BLOB, GFILE_BLOB_FILES,
				file_base, (uint32_t)aligned_sz) )
		goto err_free;
	if ( !dump_block(f, &cur, GFILE_TYPE_NIDX, GFILE_NIDX_FILENAMES,
				ovh_sz, idx_sz) )
		goto err_free;

	assert(cur == ovh_sz);

	if ( !dump_nidx(f, &cur, num_files, strtab_sz) )
		goto err_free;

	/* sort index entries by filename for fast queries */
	qsort(f_arr, num_files, sizeof(*f_arr), fcmp_name);
	for(strtab_sz = i = 0; i < num_files; i++) {
		if ( !dump_name(f, &cur,
				file_base + f_arr[i].f_ofs,
				f_arr[i].f_asz,
				strtab_base + strtab_sz,
				strlen(f_arr[i].f_name)) )
			goto err_free;
		strtab_sz += strlen(f_arr[i].f_name) + 1;
	}

	assert(cur == strtab_base);
	for(i = 0; i < num_files; i++) {
		if ( !dump_str(f, &cur, f_arr[i].f_name) )
			goto err_free;
	}

	if ( !dump_align(f, &cur) )
		goto err_free;

	assert(cur == file_base);
	qsort(f_arr, num_files, sizeof(*f_arr), fcmp_sz);
	for(i = 0; i < num_files; i++) {
		//fprintf(stderr, "[0x%.6x @ 0x%.7x]: %s\n",
		//	f_arr[i].f_rsz, cur, f_arr[i].f_name);
		assert(file_base + f_arr[i].f_ofs == cur);
		if ( !dump_file(f, &cur, &f_arr[i]) )
			return 0;
	}

	/* Job done, let's make sure it's flushed to disk */
	assert(cur == gfl_sz);
	fprintf(stderr, "Synchronising file...\n");
	if ( fsync(fileno(f)) &&
		errno != EROFS &&
		errno != EINVAL ) {
		fprintf(stderr, "fsync: %s\n", get_err());
		goto err_free;
	}

	ret = 1;

err_free:
	free(f_arr);
err:
	return ret;
}

/* Scan file size and remember it all */
static int file_to_manifest(const char *fn)
{
	struct stat st;
	struct manifest *f;

	if ( strlen(fn) + 1 > 0xffff ) {
		fprintf(stderr, "%s: name too long\n", fn);
		return 0;
	}

	if ( stat(fn, &st) ) {
		fprintf(stderr, "stat: %s: %s\n",
			fn, get_err());
		return 0;
	}

	f = mpool_alloc(&fmem);
	if ( f == NULL )
		return 0;

	f->m_name = gang_alloc(&nmem, strlen(fn) + 1);
	if ( f->m_name == NULL )
		return 0;

	strcpy(f->m_name, fn);

	f->m_size = st.st_size;
	f->m_next = manifest;
	manifest = f;
	num_files++;
	tot_sz += st.st_size;
	return 1;
}

/* Scan files to be included from the "manifest" file */
static int scan_manifest(FILE *f)
{
	char buf[8192];
	unsigned int ln;

	for(ln = 1; fgets(buf, sizeof(buf), f); ln++) {
		char *lf;

		lf = strchr(buf, '\n');
		if ( lf == NULL ) {
			fprintf(stderr, "line %u too long\n", ln);
			return 0;
		}

		*lf = '\0';
		lf = strchr(buf, '\r');
		if ( lf )
			*lf = '\0';

		if ( buf[0] == '\0' ||
			buf[0] == '#' )
			continue;

		if ( buf[0] == '/' ) {
			fprintf(stderr, "%s: absolute paths verboten\n", buf);
			return 0;
		}

		lf = buf;
		if ( strlen(buf) > 1 &&
			buf[0] == '.' && buf[1] == '/' )
				lf += 2;

		if ( !file_to_manifest(lf) )
			return 0;
	}

	return 1;
}

int main(int argc, char **argv)
{ 
	mpool_init(&fmem, sizeof(struct manifest), 0);
	gang_init(&nmem, 0);

	if ( !scan_manifest(stdin) )
		return EXIT_FAILURE;

	if ( !dump_gfile(stdout) )
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
