/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*
* Load PCX image files. PCX images are palettized RLE images.
*/
#include <blackbloc/blackbloc.h>
#include <blackbloc/gfile.h>
#include <blackbloc/tex.h>
#include <blackbloc/gl_render.h>
#include <blackbloc/img/pcx.h>

#include "teximg.h"

/* On disk format */
#define PCX_PAL 768 /* number of palette entries */
struct pcx {
	uint8_t		manufacturer;
	uint8_t		version;
	uint8_t		encoding;
	uint8_t		bits_per_pixel;
	uint16_t	xmin,ymin,xmax,ymax;
	uint16_t	hres,vres;
	uint8_t		palette[48];
	uint8_t		reserved;
	uint8_t		color_planes;
	uint16_t	bytes_per_line;
	uint16_t	palette_type;
	uint8_t		filler[58];
};

/* Internal API */
struct _pcx_img {
	struct _texture tex;
	struct gfile f;
	uint16_t width, height;
	const uint8_t *compressed;
	struct list_head list;
};

static LIST_HEAD(pcx_list);

static int prep_resample(struct _texture *tex, unsigned int mip, GLint format)
{
	uint32_t w, h;
	uint8_t *buf;

	assert(format == GL_RGBA || format == GL_BGRA);

	/* Round to powers of 2 (for OpenGL) */
	for(w = 1; w < tex->t_mipmap[mip].m_width; w <<= 1);
	for(h = 1; h < tex->t_mipmap[mip].m_height; h <<= 1);

	if ( w != tex->t_mipmap[mip].m_width || h != tex->t_mipmap[mip].m_height ) {
		buf = malloc(w * h * 4);
		if ( buf == NULL ) {
			con_printf("teximg_upload_resample: %s: malloc(): %s\n",
				tex->t_name, get_err());
			return 0;
		}

		/* Actually resample the image */
		teximg_resample(tex->t_mipmap[mip].m_pixels,
				tex->t_mipmap[mip].m_width,
				tex->t_mipmap[mip].m_height,
				buf, w, h, format);

		/* Free old image */
		free(tex->t_mipmap[mip].m_pixels);

		/* Overwrite with scaled image */
		tex->t_mipmap[mip].m_width = w;
		tex->t_mipmap[mip].m_height = h;
		tex->t_mipmap[mip].m_pixels = buf;
	}

	return 1;
}

/* Convert in to a full RGBA texture before uploading */
static int pcx_prep(struct _texture *tex, unsigned int mip)
{
	struct _pcx_img *pcx = (struct _pcx_img *)tex;
	const uint8_t *pal, *in = pcx->compressed;
	uint8_t *out;
	uint8_t cur;
	int run;
	uint32_t x,y;

	if ( mip )
		return 0;

	/* The size is checked in pcx_load */
	pal = pcx->f.f_ptr + pcx->f.f_len - PCX_PAL;

	tex->t_mipmap[mip].m_pixels = malloc(pcx->width * pcx->height * 4);
	if ( tex->t_mipmap[mip].m_pixels == NULL )
		return 0;

	tex->t_mipmap[mip].m_width = pcx->width;
	tex->t_mipmap[mip].m_height = pcx->height;

	out = tex->t_mipmap[mip].m_pixels;

	for(y = 0; y < pcx->height; y++, out += pcx->width * 4) {
		for(x = 0; x < pcx->width; ) {
			cur = *in++;

			if ( (cur & 0xc0)  == 0xc0 ) {
				run = cur & 0x3f;
				cur = *in++;
			}else{
				run = 1;
			}

			while( run-- > 0 ) {
				if ( cur != 0xff ) {
					out[4 * x + 0] = pal[3 * cur + 0];
					out[4 * x + 1] = pal[3 * cur + 1];
					out[4 * x + 2] = pal[3 * cur + 2];
					out[4 * x + 3] = 0xff;
					x++;
				}else{
					/* transparent */
					out[4 * x + 3] = 0;
					x++;
				}
			}
		}
	}

	return prep_resample(tex, mip, GL_RGBA);
}

static unsigned int pcx_width(struct _texture *tex)
{
	struct _pcx_img *pcx = (struct _pcx_img *)tex;
	return pcx->width;
}

static unsigned int pcx_height(struct _texture *tex)
{
	struct _pcx_img *pcx = (struct _pcx_img *)tex;
	return pcx->height;
}

static const struct _texops pcx_ops = {
	.prep = pcx_prep,
	.unprep = teximg_unprep_generic,
	.upload = teximg_upload_rgba,
	.dtor = teximg_dtor_generic,
	.width = pcx_width,
	.height = pcx_height,
};

static struct _texture *do_pcx_load(const char *name)
{
	struct _pcx_img *pcx;
	struct pcx hdr;

	pcx = calloc(1, sizeof(*pcx));
	if ( pcx == NULL )
		return 0;

	if ( !game_open(&pcx->f, name) )
		goto err_free;

	if ( pcx->f.f_len < sizeof(hdr) + PCX_PAL )
		goto err;

	/* Check the header out */
	memcpy(&hdr, pcx->f.f_ptr, sizeof(hdr));
	hdr.xmin = le16toh(hdr.xmin);
	hdr.ymin = le16toh(hdr.ymin);
	hdr.xmax = le16toh(hdr.xmax);
	hdr.ymax = le16toh(hdr.ymax);
	hdr.hres = le16toh(hdr.hres);
	hdr.vres = le16toh(hdr.vres);
	hdr.bytes_per_line = le16toh(hdr.bytes_per_line);
	hdr.palette_type = le16toh(hdr.palette_type);
	if ( hdr.manufacturer != 0x0a ||
		hdr.version != 5 ||
		hdr.encoding != 1 ||
		hdr.bits_per_pixel != 8 ||
		hdr.xmax >= 640 ||
		hdr.ymax >= 480 ) {
		con_printf("%s: bad PCX file\n", name);
		goto err;
	}

	pcx->width = hdr.xmax + 1;
	pcx->height = hdr.ymax + 1;
	pcx->compressed = pcx->f.f_ptr + sizeof(hdr);
	pcx->tex.t_name = pcx->f.f_name;
	pcx->tex.t_ops = &pcx_ops;

	if ( pcx->width == 0 || pcx->height == 0 )
		goto err;

	con_printf("pcx: %s (%ux%u)\n",
			pcx->tex.t_name, pcx->width, pcx->height);

	list_add_tail(&pcx->list, &pcx_list);
	tex_get(&pcx->tex);
	return &pcx->tex;
err:
	game_close(&pcx->f);
err_free:
	free(pcx);
	return NULL;
}

const uint8_t *pcx_get_colormap(pcx_img_t pcx, size_t *sz)
{
	const uint8_t *pal = pcx->f.f_ptr + pcx->f.f_len - PCX_PAL;
	if ( sz )
		*sz = PCX_PAL;
	return pal;
}

texture_t pcx_get_by_name(const char *name)
{
	struct _pcx_img *pcx;

	list_for_each_entry(pcx, &pcx_list, list) {
		if ( !strcmp(name, pcx->tex.t_name) ) {
			tex_get(&pcx->tex);
			return &pcx->tex;
		}
	}

	return do_pcx_load(name);
}
