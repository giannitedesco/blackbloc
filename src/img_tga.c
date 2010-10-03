/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*
* Load PCX image files. PCX images are palettized RLE images.
*/
#include <blackbloc/blackbloc.h>
#include <blackbloc/gfile.h>
#include <blackbloc/teximage.h>
#include <blackbloc/gl_render.h>
#include <blackbloc/img/tga.h>

static struct image *tgas;

static int tga_upload_24(struct image *i)
{
	i->mipmap[0].pixels = i->s_pixels;
	i->mipmap[0].height = i->s_height;
	i->mipmap[0].width = i->s_width;
	return img_upload_bgr(i);
}

static int tga_upload_32(struct image *i)
{
	i->mipmap[0].pixels = i->s_pixels;
	i->mipmap[0].height = i->s_height;
	i->mipmap[0].width = i->s_width;
	return img_upload_bgra(i);
}

static int tga_upload_c24(struct image *i)
{
	con_printf("TGA compressed 24bpp\n");
	return img_upload_bgra(i);
}

static int tga_upload_c32(struct image *i)
{
	uint8_t r, g, b, a;
	uint8_t *buf, *ptr, *s_ptr;
	size_t row, col;
	int count, rcount;

	con_printf("TGA compressed 32bpp\n");

	ptr = buf = malloc(i->s_height * i->s_width * 4);
	if ( NULL == buf )
		return 0;

	s_ptr = i->s_pixels;

	for(row = col = 0; row < i->s_height; ) {
		count = s_ptr[0];
		s_ptr++;
		if ( count & 0x80 )
			rcount = 1;
		count = 1 + (count & 0x7f);

		while ( count-- && row < i->s_height ) {
			if ( rcount-- > 0 ) {
				b = s_ptr[0];
				g = s_ptr[1];
				r = s_ptr[2];
				a = 0xff; //s_ptr[3];
				s_ptr += 4;
			}
			ptr[0] = r;
			ptr[1] = g;
			ptr[2] = b;
			ptr[3] = a;
			ptr += 4;
			if ( ++col == i->s_width ) {
				row++;
				col = 0;
			}
		}
	}

	i->mipmap[0].pixels = buf;
	i->mipmap[0].height = i->s_height;
	i->mipmap[0].width = i->s_width;

	return img_upload_rgba(i);
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
	switch(tga.tga_image_type) {
	case 2:
		switch(tga.tga_bpp) {
		case 24:
			i->upload = tga_upload_24;
			break;
		case 32:
			i->upload = tga_upload_32;
			break;
		default:
			goto err;
		}
		break;
	case 10:
		i->unload = img_free_unload;
		switch(tga.tga_bpp) {
		case 24:
			i->upload = tga_upload_c24;
			break;
		case 32:
			i->upload = tga_upload_c32;
			break;
		default:
			goto err;
		}
		break;
	default:
		goto err;
	}
	i->s_pixels = i->f.f_ptr + sizeof(tga) +
			tga.tga_color_map_len + tga.tga_identsize;

	if ( i->s_width == 0 || i->s_height == 0 )
		goto err;

	con_printf("tga: %s (%ix%i %ubpp)\n",
			i->name, i->s_width, i->s_height,
			tga.tga_bpp);

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
