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
#include <blackbloc/img/pcx.h>

static struct image *pcxs;

/* Convert in to a full RGBA texture before uploading */
static int pcx_upload(struct image *i)
{
	const uint8_t *pal, *in = i->s_pixels;
	uint8_t *out;
	uint8_t cur;
	int run;
	uint32_t x,y;

	/* The size is checked in pcx_load */
	pal = i->f.f_ptr + i->f.f_len - PCX_PAL;

	i->mipmap[0].pixels = malloc(i->s_width * i->s_height * 4);
	if ( i->mipmap[0].pixels == NULL )
		return 0;

	i->mipmap[0].width = i->s_width;
	i->mipmap[0].height = i->s_height;

	out = i->mipmap[0].pixels;

	for(y = 0; y < i->s_height; y++, out += i->s_width * 4) {
		for(x = 0; x < i->s_width; ) {
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

	return img_upload_resample(i, GL_RGBA);
}

static int do_pcx_load(const char *name, uint8_t *map)
{
	struct image *i;
	struct pcx pcx;

	i = calloc(1, sizeof(*i));
	if ( i == NULL )
		return 0;

	if ( !game_open(&i->f, name) )
		goto err_free;

	if ( i->f.f_len < sizeof(pcx) + PCX_PAL )
		goto err;

	/* Check the header out */
	memcpy(&pcx, i->f.f_ptr, sizeof(pcx));
	pcx.xmin = le_16(pcx.xmin);
	pcx.ymin = le_16(pcx.ymin);
	pcx.xmax = le_16(pcx.xmax);
	pcx.ymax = le_16(pcx.ymax);
	pcx.hres = le_16(pcx.hres);
	pcx.vres = le_16(pcx.vres);
	pcx.bytes_per_line = le_16(pcx.bytes_per_line);
	pcx.palette_type = le_16(pcx.palette_type);
	if ( pcx.manufacturer != 0x0a ||
		pcx.version != 5 ||
		pcx.encoding != 1 ||
		pcx.bits_per_pixel != 8 ||
		pcx.xmax >= 640 ||
		pcx.ymax >= 480 ) {
		con_printf("%s: bad PCX file\n", name);
		goto err;
	}

	i->s_width = pcx.xmax + 1;
	i->s_height = pcx.ymax + 1;
	i->s_pixels = i->f.f_ptr + sizeof(pcx);
	i->name = i->f.f_name;
	i->upload = pcx_upload;
	i->unload = img_free_unload;

	if ( i->s_width == 0 || i->s_height == 0 )
		goto err;

	/* Palette copy mode */
	if ( map ) {
		const uint8_t *pal = i->f.f_ptr + i->f.f_len - PCX_PAL;
		memcpy(map, pal, PCX_PAL);
		game_close(&i->f);
		free(i);
		return 1;
	}

	con_printf("pcx: %s (%ix%i)\n",
			i->name, i->s_width, i->s_height);

	i->next = pcxs;
	pcxs = i;

	return 1;
err:
	game_close(&i->f);
err_free:
	free(i);
	return 0;
}

struct image *pcx_get_by_name(const char *n)
{
	struct image *ret;
	int again = 0;

try_again:
	for(ret = pcxs; ret; ret = ret->next) {
		if ( !strcmp(n, ret->name) ) {
			img_get(ret);
			return ret;
		}
	}

	if ( again )
		return NULL;

	/* load from textures/name.wal */
	if ( !pcx_load(n) )
		return NULL;

	again = 1;
	goto try_again;
}

int pcx_load_colormap(const char *name, uint8_t *map)
{
	return do_pcx_load(name, map);
}

int pcx_load(const char *name)
{
	return do_pcx_load(name, NULL);
}
