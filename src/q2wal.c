/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*
* Load QuakeII WAL files (wall textures used in BSP maps). We depend on
* the PCX loader because we need it to load our palette in.
*/
#include <SDL.h>

#include <blackbloc/blackbloc.h>
#include <blackbloc/gfile.h>
#include <blackbloc/teximage.h>
#include <blackbloc/gl_render.h>
#include <blackbloc/img/q2wal.h>
#include <blackbloc/img/pcx.h>

static struct image *wals;
static uint8_t palette[PCX_PAL];
static int got_palette;

/* Load from symbolic name rather than filename */
int q2wal_load_by_name(const char *name)
{
	size_t nlen = strlen(name) + 14;
	char buf[nlen];

	/* Check if it already loaded */
	if ( q2wal_find(name) )
		return 0;

	/* If not, grab it */
	snprintf(buf, nlen, "textures/%s.wal", name);
	return q2wal_load(buf);
}

struct image *q2wal_find(const char *n)
{
	struct image *ret;

	for(ret = wals; ret; ret = ret->next) {
		if ( !strcmp(n, ret->name) )
			return ret;
	}

	return NULL;
}

/* Get an image provided its name (eg: "e1u1/floor1_3") */
struct image *q2wal_get(const char *n)
{
	struct image *ret;
	int again = 0;

try_again:
	if ( (ret = q2wal_find(n)) ) {
		img_get(ret);
		return ret;
	}else if ( again ) {
		return NULL;
	}

	if ( !q2wal_load_by_name(n) )
		return NULL;

	again++;

	goto try_again;
}

/* Use the colormap.pcx to translate from 8bit paletized WAL to
 * full RGB texture, gah */
static int q2wal_upload(struct image *wal)
{
	int width = wal->s_width;
	int height = wal->s_height;
	unsigned char *trans;
	const unsigned char *data = wal->s_pixels;
	int s, i, p;

	wal->mipmap[0].pixels = malloc(width * height * 3);
	if ( wal->mipmap[0].pixels == NULL )
		return 0;

	trans = wal->mipmap[0].pixels;

	s = width * height;

	for(i = 0; i < s; i++) {
		p = data[i];

		if ( p == 0xff ) {
			if (i > width && data[i - width] != 0xff)
				p = data[i - width];
			else if (i < s-width && data[i + width] != 0xff)
				p = data[i + width];
			else if (i > 0 && data[i - 1] != 0xff)
				p = data[i - 1];
			else if (i < s - 1 && data[i + 1] != 0xff)
				p = data[i + 1];
			else
				p = 0;
		}

		*trans++ = palette[3 * p + 0];
		*trans++ = palette[3 * p + 1];
		*trans++ = palette[3 * p + 2];
	}

	wal->mipmap[0].width = wal->s_width;
	wal->mipmap[0].height = wal->s_height;

	img_upload_rgb(wal);

	return 1;
}

/* Load in a WAL */
int q2wal_load(const char *name)
{
	struct miptex *t;
	struct image *wal;
	size_t ofs;

	/* Try load the palette if we don't have it */
	if ( !got_palette ) {
		if ( !pcx_load_colormap("pics/colormap.pcx", palette) ) {
			con_printf("q2wal: error loading palette\n");
			return 0;
		}

		got_palette = 1;
	}

	wal = calloc(1, sizeof(*wal));
	if ( wal == NULL )
		return 0;

	if ( !game_open(&wal->f, name) )
		goto err_free;

	if ( wal->f.f_len < sizeof(*t) ) {
		con_printf("%s: short wal\n", name);
		goto err;
	}

	t = (struct miptex *)wal->f.f_ptr;

	/* Fill in the details from the header */
	wal->name = t->name;
	ofs = le32toh(t->offsets[0]);
	wal->s_width = le32toh(t->width);
	wal->s_height = le32toh(t->height);
	wal->s_pixels = wal->f.f_ptr + ofs;
	wal->mipmap[0].pixels = NULL;
	wal->upload = q2wal_upload;
	wal->unload = img_free_unload;

	/* Sanity checks on pixel data */
	if ( wal->s_width == 0 || wal->s_height == 0 ) {
		con_printf("%s: corrupt wal\n", name);
		goto err;
	}

	if ( ofs + (wal->s_width * wal->s_height) > wal->f.f_len ) {
		con_printf("%s: truncated wal\n", name);
		goto err;
	}

	wal->next = wals;
	wals = wal;

#if 0
	con_printf("q2wal: %s (%s) (%dx%d)\n",
		name, wal->name, wal->s_width, wal->s_height);
#endif

	return 1;
err:
	game_close(&wal->f);
err_free:
	free(wal);
	return 0;
}
