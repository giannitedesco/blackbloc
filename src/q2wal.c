/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*
* Load QuakeII WAL files (wall textures used in BSP maps). We depend on
* the PCX loader because we need it to load our palette in.
*/
#include <SDL.h>

#include <blackbloc.h>
#include <gfile.h>
#include <teximage.h>
#include <gl_render.h>
#include <q2wal.h>
#include <img_pcx.h>

static struct image *wals;
static uint8_t palette[PCX_PAL];
static int got_palette;

/* Load from symbolic name rather than filename */
int q2wal_load_by_name(char *name)
{
	size_t nlen=strlen(name)+14;
	char buf[nlen];

	/* Check if it already loaded */
	if ( q2wal_find(name) )
		return 0;

	/* If not, grab it */
	snprintf(buf, nlen, "textures/%s.wal", name);
	return q2wal_load(buf);
}

struct image *q2wal_find(char *n)
{
	struct image *ret;

	for(ret=wals; ret; ret=ret->next) {
		if ( !strcmp(n, ret->name) )
			return ret;
	}

	return NULL;
}

/* Get an image provided its name (eg: "e1u1/floor1_3") */
struct image *q2wal_get(char *n)
{
	struct image *ret;
	int again=0;

try_again:
	if ( (ret=q2wal_find(n)) ) {
		img_get(ret);
		return ret;
	}else if ( again ) {
		return NULL;
	}

	if ( q2wal_load_by_name(n) )
		return NULL;

	again++;

	goto try_again;
}

/* Use the colormap.pcx to translate from 8bit paletized WAL to
 * full RGB texture, gah */
static int q2wal_upload(struct image *wal)
{
	int width=wal->s_width;
	int height=wal->s_height;
	unsigned char *trans;
	unsigned char *data=wal->s_pixels;
	int s, i, p;

	if ( !(wal->mipmap[0].pixels=malloc(width*height*3)) )
		return -1;

	trans=wal->mipmap[0].pixels;

	s=width * height;

	for(i=0; i<s; i++) {
		p=data[i];

		if (p == 255) {
			if (i > width && data[i-width] != 255)
				p = data[i-width];
			else if (i < s-width && data[i+width] != 255)
				p = data[i+width];
			else if (i > 0 && data[i-1] != 255)
				p = data[i-1];
			else if (i < s-1 && data[i+1] != 255)
				p = data[i+1];
			else
				p = 0;
		}

		*trans++ = palette[3*p+0];
		*trans++ = palette[3*p+1];
		*trans++ = palette[3*p+2];
	}

	wal->mipmap[0].width=wal->s_width;
	wal->mipmap[0].height=wal->s_height;

	img_upload_rgb(wal);

	return 0;
}

/* Load in a WAL */
int q2wal_load(char *name)
{
	struct miptex *t;
	struct image *i;
	size_t ofs;

	/* Try load the palette if we don't have it */
	if ( !got_palette ) {
		if ( pcx_load_colormap("pics/colormap.pcx", palette)<0 ) {
			con_printf("q2wal: error loading palette\n");
			return -1;
		}

		got_palette=1;
	}

	if ( !(i=calloc(1, sizeof(*i))) )
		return -1;

	if ( gfile_open(&i->f, name)<0 ) {
		free(i);
		return -1;
	}

	if ( i->f.len<sizeof(*t) ) {
		con_printf("%s: short wal\n", name);
		goto err;
	}

	t=(struct miptex *)i->f.data;

	/* Fill in the details from the header */
	i->name=t->name;
	ofs=le_32(t->offsets[0]);
	i->s_width=le_32(t->width);
	i->s_height=le_32(t->height);
	i->s_pixels=i->f.data+ofs;
	i->mipmap[0].pixels=NULL;
	i->upload=q2wal_upload;
	i->unload=img_free_unload;

	/* Sanity checks on pixel data */
	if ( i->s_width==0 || i->s_height==0 ) {
		con_printf("%s: corrupt wal\n", name);
		goto err;
	}

	if ( ofs+(i->s_width*i->s_height) > i->f.len ) {
		con_printf("%s: truncated wal\n", name);
		goto err;
	}

	i->next=wals;
	wals=i;

#if 0
	con_printf("q2wal: %s (%s) (%ix%i)\n",
		name, i->name, i->s_width, i->s_height);
#endif

	return 0;
err:
	gfile_close(&i->f);
	free(i);
	return -1;
}
