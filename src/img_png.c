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
#include <blackbloc/teximage.h>
#include <blackbloc/gl_render.h>
#include <blackbloc/img/png.h>

static struct image *pngs;
struct png_io_ffs {
	struct gfile *file;
	size_t fptr;
};

/* Replacement read function for libpng */
static void png_read_data_fn(png_structp png, png_bytep data, png_size_t len)
{
	struct png_io_ffs *ffs = png_get_io_ptr(png);

	if ( ffs->fptr > ffs->file->f_len )
		return;
	if ( ffs->fptr + len > ffs->file->f_len )
		len = ffs->file->f_len - ffs->fptr;
	memcpy(data, ffs->file->f_ptr + ffs->fptr, len);
	ffs->fptr += len;
}

static int png_load(const char *name)
{
	struct image *i;
	png_structp png;
	png_infop info = NULL;
	int bits, color, inter;
	png_uint_32 w, h;
	uint8_t *buf;
	unsigned int x, rb;
	struct png_io_ffs ffs;

	if ( !(png=png_create_read_struct(PNG_LIBPNG_VER_STRING,
						NULL, NULL, NULL)) ) {
		goto err;
	}

	if ( !(info=png_create_info_struct(png)) )
		goto err_png;

	if ( !(i=calloc(1, sizeof(*i))) )
		goto err_png;

	if ( !game_open(&i->f, name) )
		goto err_img;

	if ( i->f.f_len < 8 || png_sig_cmp((void *)i->f.f_ptr, 0, 8) ) {
		con_printf("png: %s: bad signature\n", name);
		goto err_close;
	}

	ffs.file = &i->f;
	ffs.fptr = 0;
	png_set_read_fn(png, &ffs, png_read_data_fn);
	png_read_info(png, info);

	/* Get the information we need */
	png_get_IHDR(png, info, &w, &h, &bits,
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
		con_printf("png: %s: bad format (%ux%ux%ibit) %i\n",
			name, w, h, bits, color);
		goto err_close;
	}

	/* Allocate buffer and read image */
	rb = png_get_rowbytes(png, info);
	buf = malloc(h * rb);
	for(x=0; x < h; x++) {
		png_read_row(png, buf + (x*rb), NULL);
	}

	i->name = i->f.f_name;
	i->mipmap[0].width = i->s_width = w;
	i->mipmap[0].height = i->s_height = h;
	i->s_pixels = i->mipmap[0].pixels = buf;
	i->unload = img_free_unload;
	if ( color == PNG_COLOR_TYPE_RGB_ALPHA )
		i->upload = img_upload_rgba;
	if ( color == PNG_COLOR_TYPE_RGB )
		i->upload = img_upload_rgb;

	png_read_end(png, info);
	png_destroy_read_struct(&png, &info, NULL);
	game_close(&i->f);

	i->next = pngs;
	pngs = i;

	return 1;

err_close:
	game_close(&i->f);
err_img:
	free(i);
err_png:
	png_destroy_read_struct(&png, &info, NULL);
err:
	return 0;
}

struct image *png_get_by_name(char *n)
{
	struct image *ret;
	int again=0;

try_again:
	for(ret=pngs; ret; ret=ret->next) {
		if ( !strcmp(n, ret->name) ) {
			img_get(ret);
			return ret;
		}
	}

	if ( again )
		return NULL;

	if ( !png_load(n) )
		return NULL;

	again=1;
	goto try_again;
}
