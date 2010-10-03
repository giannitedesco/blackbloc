/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __TEXIMAGE_HEADER_INCLUDED__
#define __TEXIMAGE_HEADER_INCLUDED__

#define MIPLEVELS 4

struct image {
	const char *name;
	int ref;
	GLuint texnum;

	struct image *next;
	int(*upload)(struct image *i);
	void(*unload)(struct image *i);

	/* source image */
	struct gfile f;
	uint32_t s_width, s_height;
	const uint8_t *s_pixels;

	/* Final scaled image */
	struct {
		uint8_t *pixels;
		unsigned int width, height;
	}mipmap[MIPLEVELS];
};

int img_upload_generic(struct image *img, GLint format);
int img_upload_rgb(struct image *img);
int img_upload_bgr(struct image *img);
int img_upload_rgba(struct image *img);
int img_upload_rgb2rgba(struct image *img);
void img_free_unload(struct image *img);
void img_put(struct image *img);
void img_get(struct image *img);
void img_bind(struct image *img);

#endif /* __TEXIMAGE_HEADER_INCLUDED__ */
