/*
* This file is part of blackbloc
* Copyright (c) 2008 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#include <blackbloc/blackbloc.h>
#include <blackbloc/gfile.h>
#include <blackbloc/gfile-internal.h>

#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#define GFS_MAX_LEN (0xffffffff)
#define GFS_MIN_LEN (sizeof(struct gfile_hdr) + \
			2 * sizeof(struct gfile_blk) + \
			sizeof(struct gfile_nidx))

/* Note that the file layout is such that we only need
 * these 4 fields to do correct + optimnal file lookup
 */
struct _gfs {
	const uint8_t *fs_map;
	const uint8_t *fs_end;
	uint32_t fs_num_names;
	const struct gfile_name *fs_name;
};

static const struct gfile_nidx *gfile_nidx(struct _gfs *fs,
					const struct gfile_blk *b)
{
	const struct gfile_nidx *n;
	const uint8_t *ptr;
	size_t ofs;

	ofs = be_32(b->b_ofs);

	if ( fs->fs_map + ofs + sizeof(*n) > fs->fs_end )
		return NULL;

	ptr = (fs->fs_map + ofs);
	n = (struct gfile_nidx *)ptr;

	n += sizeof(struct gfile_name) * be_32(n->i_num_names);
	n += be_32(n->i_strtab_sz);
	if ( (uint8_t *)n > fs->fs_end )
		return NULL;

	n = (const struct gfile_nidx *)ptr;
	return n;
}

static struct gfile_blk *gfile_blk(struct _gfs *fs,
					unsigned int type,
					unsigned int id)
{
	unsigned int i;
	struct gfile_hdr *h;
	struct gfile_blk *b;
	unsigned int num_blk;

	h = (struct gfile_hdr *)fs->fs_map;
	num_blk = be_32(h->h_num_blk);

	b = h->h_blk + num_blk;
	if ( (uint8_t *)b > fs->fs_end )
		return NULL;

	/* linear scan for now */
	for(b = h->h_blk, i = 0; i < num_blk; i++,b++) {
		if ( be_16(b->b_type) != type )
			continue;
		if ( be_16(b->b_id) != id )
			continue;
		return b;
	}

	return NULL;
}

gfs_t gfs_open(const char *fn)
{
	const struct gfile_hdr *h;
	const struct gfile_blk *b;
	const struct gfile_nidx *n;
	struct _gfs *fs;
	struct stat st;
	size_t fs_len;
	int fd;

	fs = calloc(1, sizeof(*fs));
	if (fs == NULL)
		goto out;

	fd = open(fn, O_RDONLY);
	if ( fd < 0 )
		goto err_free;

	if ( fstat(fd, &st) )
		goto err_close;

#if _FILE_OFFSET_BITS > 32
	if ( st.st_size > GFS_MAX_LEN )
		goto err_close;
#endif
	if ( (size_t)st.st_size < GFS_MIN_LEN )
		goto err_close;

	fs_len = st.st_size;
	fs->fs_map = mmap(NULL, fs_len, PROT_READ, MAP_SHARED, fd, 0);
	if ( fs->fs_map == MAP_FAILED )
		goto err_close;

	fs->fs_end  = fs->fs_map + fs_len;
	h = (const struct gfile_hdr *)fs->fs_map;
	if ( be_32(h->h_magic) != GFILE_MAGIC )
		goto err_unmap;
	
	/* The DB is basically "live" now as far as accessor methods
	 * are concerned...
	 */

	b = gfile_blk(fs, GFILE_TYPE_NIDX, GFILE_NIDX_FILENAMES);
	if ( b == NULL )
		goto err_unmap;

	n = gfile_nidx(fs, b);
	if ( n == NULL )
		goto err_unmap;
	
	fs->fs_num_names = be_32(n->i_num_names);
	fs->fs_name = n->i_name;

	con_printf("%s: %s: %u files\n",
			_func, fn, fs->fs_num_names);

	close(fd);
	return fs;
err_unmap:
	munmap((void *)fs->fs_map, fs_len);
err_close:
	close(fd);
err_free:
	free(fs);
out:
	con_printf("%s: %s: %s\n", _func, fn, load_err("File corrupted"));
	return NULL;
}

void gfs_close(gfs_t fs)
{
	munmap((void *)fs->fs_map, fs->fs_end - fs->fs_map);
	free(fs);
}

int gfile_open(gfs_t fs, struct gfile *f, const char *name)
{
	const struct gfile_name *ptr;
	unsigned int n;

	assert(fs != NULL);
	assert(f != NULL);

	n = fs->fs_num_names;
	ptr = fs->fs_name;

	for(;;) {
		unsigned int i;
		int ret;

		i = n / 2;
		ret = strcmp(name,
				(char *)fs->fs_map + be_32(ptr[i].n_name));
		if ( ret < 0 ) {
			n = i;
		}else if ( ret > 0 ) {
			ptr = ptr + (i + 1);
			n = n - (i + 1);
		}else{
			f->f_ptr = fs->fs_map + be_32(ptr[i].n_ofs);
			f->f_len = be_32(ptr[i].n_len);
			f->f_name = (char *)fs->fs_map + be_32(ptr[i].n_name);
			assert((uint8_t *)f->f_ptr + f->f_len < fs->fs_end);
			return 1;
		}
		if ( n == 0 )
			break;
	}

	return 0;
}

void gfile_close(gfs_t fs, struct gfile *f)
{
	assert(fs != NULL);
	assert(f != NULL);
	/* HEH, that was easy */
}
