/*
* This file is part of blackbloc
* Copyright (c) 2002 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*
* Reference counted zero-copy memory mapped quake2
* pakfile API.
*
* TODO:
*  o MTF heuristic
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include <blackbloc.h>
#include <gfile.h>
#include <q2pak.h>

static void q2pak_put(struct pak_rec *r);
static void q2pak_closefile(struct gfile *f);

static struct gfile_ops q2pak_ops={
	.close = q2pak_closefile,
	.open = q2pak_open,
	.type_name = "q2pak",
	.type_desc = "QuakeII PAK file",
};

/* Hash table of all names -> files */
static struct pak_rec *pakhash[PAKHASH];

/* Link a new file in to the hashtable, overriding an older one if
 * necessary
 */
static void q2pak_link(struct pak_rec *r)
{
	struct pak_rec *prev=NULL;
	struct pak_rec *t;
	int bucket;

	/* Link count is one for being linked in to the lookup table */
	r->owner->use++;

	bucket = r->hash % PAKHASH;

	for(t=pakhash[bucket]; t; t=t->next) {
		if ( r->hash == t->hash &&
			!strcmp(t->name, r->name) ) {
			break;
		}
		prev = t;
	}

	if ( t ) {
		r->next = t->next;
		if ( prev )
			prev->next = r;
		else
			pakhash[bucket] = r;

		q2pak_put(t);
	}else{
		r->next = pakhash[bucket];
		pakhash[bucket] = r;
	}
}

/* Add a pakfile to the lookup table */
void q2pak_add(char *fn)
{
	struct pak_dhdr hdr;
	struct pak_dfile *files;
	struct pak *p;
	struct stat st;
	char *end;
	int i;
	int failed=0;

	if ( !(p=calloc(1, sizeof(*p))) ) {
		con_printf("q2pak: %s: calloc(): %s\n", fn, get_err());
		return;
	}

	if ( !(p->name = strdup(fn)) ) {
		con_printf("q2pak: %s: strdup(): %s\n", fn, get_err());
		goto err;
	}

	if ( (p->fd=open(fn, O_RDONLY))<0 ) {
		con_printf("q2pak: %s: open(): %s\n", fn, get_err());
		goto err_free;
	}

	if ( fstat(p->fd, &st) ) {
		con_printf("q2pak: %s: fstat(): %s\n", fn, get_err());
		goto err_close;
	}

	if ( st.st_size < (loff_t)sizeof(hdr) ) {
		con_printf("file is too small for header!\n");
		goto err_close;
	}

	if ( (p->map=mmap(NULL, st.st_size, PROT_READ,
		MAP_SHARED, p->fd, 0))==MAP_FAILED ) {
		con_printf("q2pak: %s: mmap(): %s\n", fn, get_err());
		goto err_close;
	}

	p->maplen=st.st_size;
	end=p->map+st.st_size;

	/* File is now fully mapped in, so lets build
	 * the filename -> data hashtable, this will
	 * speed up file loading. */

	/* First check the header */
	memcpy(&hdr, p->map, sizeof(hdr));
	hdr.ident=le_32(hdr.ident);
	hdr.dirofs=le_32(hdr.dirofs);
	hdr.dirlen=le_32(hdr.dirlen);

	p->numfiles=hdr.dirlen/sizeof(struct pak_dfile);
	files=(struct pak_dfile *)(p->map+hdr.dirofs);

	if ( hdr.ident != IDPAKHEADER ) {
		con_printf("%s: not a packfile\n", fn);
		goto err_unmap;
	}

	if ( hdr.dirofs + hdr.dirlen > st.st_size ) {
		con_printf("%s: truncated file list\n", fn);
		goto err_unmap;
	}

	/* Allocate all pak_rec structures */
	if ( !(p->chunk=calloc(p->numfiles, sizeof(*p))) ) {
		goto err_unmap;
	}

	/* Put all files in the hash table */
	for(i=0; i<p->numfiles; i++) {
		struct pak_rec *r=p->chunk + i;

		/* TODO: Check termination of name */
		r->name=files[i].name;
		r->data=p->map+le_32(files[i].filepos);
		r->len=le_32(files[i].filelen);
		r->owner=p;
		r->hash=skunk_hash(files[i].name);

		/* Check data is readable */
		if ( r->data + r->len > end ) {
			con_printf("%s: error loading %s\n",
				fn, r->name);
			failed++;
			continue;
		}

		/* link to hash table */
		q2pak_link(r);
	}

	p->numfiles-=failed;

	con_printf("q2pak: %s: loaded %u/%u files in pak\n",
		fn, p->numfiles, p->numfiles+failed);

	return;

	/* error paths */
err_unmap:
	munmap(p->map, p->maplen);
err_close:
	close(p->fd);
err_free:
	free(p->name);
err:
	free(p);
	return;
}

/* Open a file */
int q2pak_open(struct gfile *f, const char *path)
{
	skunk_hash_t hash;
	struct pak_rec *r;

	hash=skunk_hash(path);

	for(r=pakhash[hash%PAKHASH]; r; r=r->next) {
		if ( r->hash==hash && !strcmp(r->name, path) ) {
			r->owner->use++;
			f->data=r->data;
			f->len=r->len;
			f->name=r->name;
			f->ops=&q2pak_ops;
			f->priv=r;
			return 0;
		}
	}

	return -1;
}

/* Remove a pak file when all of its files have been overridden */
void q2pak_close(struct pak *pak)
{
	if ( pak->use ) {
		con_printf("q2pak: %s: freeing when use count is %i\n",
			pak->name, pak->use);
		return;
	}
	free(pak->name);
	free(pak->chunk);
	munmap(pak->map, pak->maplen);
	close(pak->fd);
	free(pak);
}

/* Reduce usage count on pakfile holding a file */
static void q2pak_put(struct pak_rec *r)
{
	r->owner->use--;

	if ( r->owner->use == 0 )
		q2pak_close(r->owner);
}

/* gfile close operation */
static void q2pak_closefile(struct gfile *f)
{
	q2pak_put(f->priv);
}
