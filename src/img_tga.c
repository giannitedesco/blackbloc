/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*
* Load PCX image files. PCX images are palettized RLE images.
*/
#include <blackbloc.h>
#include <gfile.h>
#include <teximage.h>
#include <gl_render.h>
#include <img_tga.h>

static struct image *tgas;

/* Convert in to a full RGBA texture before uploading */
static int tga_upload(struct image *i)
{
	i->mipmap[0].pixels = i->s_pixels;
	i->mipmap[0].height = i->s_height;
	i->mipmap[0].width = i->s_width;
	return img_upload_bgr(i);
}

int tga_load(const char *name)
{
	struct image *i;
	struct tga tga;

	i = calloc(1, sizeof(*i));
	if ( i == NULL )
		return 0;

	if ( !game_open(&i->f, name) )
		goto err_free;

	if ( i->f.f_len < sizeof(tga) )
		goto err;

	/* Check the header out */
	memcpy(&tga, i->f.f_ptr, sizeof(tga));

	tga.tga_color_map_start = le16toh(tga.tga_color_map_start);
	tga.tga_color_map_len = le16toh(tga.tga_color_map_len);
	tga.tga_xstart = le16toh(tga.tga_xstart);
	tga.tga_ystart = le16toh(tga.tga_ystart);
	tga.tga_width = le16toh(tga.tga_width);
	tga.tga_height = le16toh(tga.tga_height);

	i->s_width = tga.tga_width;
	i->s_height = tga.tga_height;
	i->name = i->f.f_name;
	i->upload = tga_upload;
	i->s_pixels = i->f.f_ptr + sizeof(tga) +
			tga.tga_color_map_len + tga.tga_identsize;

	if ( i->s_width == 0 || i->s_height == 0 )
		goto err;

	con_printf("tga: %s (%ix%i %ubps)\n",
			i->name, i->s_width, i->s_height,
			tga.tga_bps);

	i->next = tgas;
	tgas = i;

	return 1;
err:
	game_close(&i->f);
err_free:
	free(i);
	return 0;
}

struct image *tga_get_by_name(const char *n)
{
	struct image *ret;
	int again = 0;

try_again:
	for(ret = tgas; ret; ret = ret->next) {
		if ( !strcmp(n, ret->name) ) {
			img_get(ret);
			return ret;
		}
	}

	if ( again )
		return NULL;

	/* load from textures/name.wal */
	if ( !tga_load(n) )
		return NULL;

	again = 1;
	goto try_again;
}
