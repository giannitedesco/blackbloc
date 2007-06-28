/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __TEXIMAGE_HEADER_INCLUDED__
#define __TEXIMAGE_HEADER_INCLUDED__

#define MIPLEVELS 4

struct image {
	char *name;
	int ref;
	GLuint texnum;

	struct image *next;
	int(*upload)(struct image *i);
	void(*unload)(struct image *i);

	/* source image */
	struct gfile f;
	unsigned int s_width, s_height;
	uint8_t *s_pixels;

	/* Final scaled image */
	struct {
		uint8_t *pixels;
		unsigned int width, height;
	}mipmap[MIPLEVELS];

	u_int32_t flags;
};

int img_upload_generic(struct image *i, GLint format);
int img_upload_rgb(struct image *);
int img_upload_rgba(struct image *);
int img_upload_rgb2rgba(struct image *);
void img_free_unload(struct image *);

static inline void img_get(struct image *i)
{
	if ( i->ref == 0 ) {
		glGenTextures(1, &i->texnum);
		if ( !i->upload(i) ) {
			/* TODO: Use default texture */
		}
	}

	i->ref++;
}

static inline void img_put(struct image *i)
{
	i->ref--;
	if ( i->ref == 0 )
		glDeleteTextures(1, &i->texnum);
}

static inline void img_bind(struct image *i)
{
	glBindTexture(GL_TEXTURE_2D, i->texnum);
}

#endif /* __TEXIMAGE_HEADER_INCLUDED__ */
