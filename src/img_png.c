/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*
* Load PNG image files. PNG is a standard image format which
* supports lossless compression and alpha channel amongst other
* nifty features.
*
* TODO:
*  o Libpng error handling crap
*/
#include <png.h>

#include <blackbloc/blackbloc.h>
#include <blackbloc/gfile.h>
#include <blackbloc/tex.h>
#include <blackbloc/gl_render.h>
#include <blackbloc/img/png.h>

#include "teximg.h"

struct _png_img {
	struct _texture tex;
	struct gfile f;
	png_uint_32 width, height;
	uint8_t *pixels;
	struct list_head list;
};

static LIST_HEAD(png_list);

struct png_io_ffs {
	struct gfile *file;
	size_t fptr;
};

static unsigned int png_width(struct _texture *tex)
{
	struct _png_img *png = (struct _png_img *)tex;
	return png->width;
}

static unsigned int png_height(struct _texture *tex)
{
	struct _png_img *png = (struct _png_img *)tex;
	return png->height;
}

static int prep(struct _texture *tex, unsigned int mip)
{
	struct _png_img *png = (struct _png_img *)tex;
	if ( mip )
		return 0;
	png->tex.t_mipmap[0].m_pixels = (uint8_t *)png->pixels;
	png->tex.t_mipmap[0].m_height = png->height;
	png->tex.t_mipmap[0].m_width = png->width;
	return 1;
}

static const struct _texops ops_rgb = {
	.prep = prep,
	.upload = teximg_upload_rgb,
	.dtor = teximg_dtor_generic,
	.width = png_width,
	.height = png_height,
};

static const struct _texops ops_rgba = {
	.prep = prep,
	.upload = teximg_upload_rgba,
	.dtor = teximg_dtor_generic,
	.width = png_width,
	.height = png_height,
};

/* Replacement read function for libpng */
static void png_read_data_fn(png_structp pngstruct, png_bytep data, png_size_t len)
{
	struct png_io_ffs *ffs = png_get_io_ptr(pngstruct);

	if ( ffs->fptr > ffs->file->f_len )
		return;
	if ( ffs->fptr + len > ffs->file->f_len )
		len = ffs->file->f_len - ffs->fptr;
	memcpy(data, ffs->file->f_ptr + ffs->fptr, len);
	ffs->fptr += len;
}

static struct _texture *do_png_load(const char *name)
{
	struct _png_img *png;
	png_structp pngstruct;
	png_infop info = NULL;
	int bits, color, inter;
	png_uint_32 w, h;
	uint8_t *buf;
	unsigned int x, rb;
	struct png_io_ffs ffs;

	if ( !(pngstruct=png_create_read_struct(PNG_LIBPNG_VER_STRING,
						NULL, NULL, NULL)) ) {
		goto err;
	}

	if ( !(info=png_create_info_struct(pngstruct)) )
		goto err_png;

	if ( !(png=calloc(1, sizeof(*png))) )
		goto err_png;

	if ( !game_open(&png->f, name) )
		goto err_img;

	if ( png->f.f_len < 8 || png_sig_cmp((void *)png->f.f_ptr, 0, 8) ) {
		con_printf("pngstruct: %s: bad signature\n", name);
		goto err_close;
	}

	ffs.file = &png->f;
	ffs.fptr = 0;
	png_set_read_fn(pngstruct, &ffs, png_read_data_fn);
	png_read_info(pngstruct, info);

	/* Get the information we need */
	png_get_IHDR(pngstruct, info, &w, &h, &bits,
			&color, &inter, NULL, NULL);

	/* o Must be power of two in dimensions (for OpenGL)
	 * o Must be 8 bits per pixel
	 * o Must not be interlaced
	 * o Must be either RGB or RGBA
	 */
	if ( (w & (w-1)) || (h & (h-1) ) || bits != 8 || 
		inter != PNG_INTERLACE_NONE ||
		(color != PNG_COLOR_TYPE_RGB &&
		 color != PNG_COLOR_TYPE_RGB_ALPHA) ) {
		con_printf("pngstruct: %s: bad format (%ux%ux%ibit) %i\n",
			name, w, h, bits, color);
		goto err_close;
	}

	/* Allocate buffer and read image */
	rb = png_get_rowbytes(pngstruct, info);
	buf = malloc(h * rb);
	if ( NULL == buf )
		goto err_close;

	for(x = 0; x < h; x++) 
		png_read_row(pngstruct, buf + (x*rb), NULL);

	png->tex.t_name = png->f.f_name;
	png->height = h;
	png->width = w;
	png->pixels = buf;
	if ( color == PNG_COLOR_TYPE_RGB_ALPHA )
		png->tex.t_ops = &ops_rgba;
	if ( color == PNG_COLOR_TYPE_RGB )
		png->tex.t_ops = &ops_rgb;

	png_read_end(pngstruct, info);
	png_destroy_read_struct(&pngstruct, &info, NULL);
	game_close(&png->f);

	list_add_tail(&png->list, &png_list);

	tex_get(&png->tex);
	return &png->tex;

err_close:
	game_close(&png->f);
err_img:
	free(png);
err_png:
	png_destroy_read_struct(&pngstruct, &info, NULL);
err:
	return NULL;
}


texture_t png_get_by_name(const char *name)
{
	struct _png_img *png;

	list_for_each_entry(png, &png_list, list) {
		if ( !strcmp(name, png->tex.t_name) ) {
			tex_get(&png->tex);
			return &png->tex;
		}
	}

	return do_png_load(name);
}
