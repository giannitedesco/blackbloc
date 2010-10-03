/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __TEXIMG_INTERNAL_HEADER_INCLUDED__
#define __TEXIMG_INTERNAL_HEADER_INCLUDED__

#define MAX_MIPLEVELS 4

struct _texops {
	int (*prep)(texture_t tex, unsigned int mip);
	int (*upload)(texture_t tex);
	void (*unprep)(texture_t tex, unsigned int mip);
	unsigned int (*width)(texture_t tex);
	unsigned int (*height)(texture_t tex);
	void (*dtor)(texture_t tex);
};

struct _texture {
	const char *t_name;
	const struct _texops *t_ops;
	unsigned int t_ref;
	unsigned char t_miplevels;
	unsigned char t_uploaded;
	GLuint t_texnum;
	struct {
		uint8_t *m_pixels;
		unsigned int m_width, m_height;
	}t_mipmap[MAX_MIPLEVELS];
};

void teximg_dtor_generic(texture_t tex);

int teximg_upload_generic(struct _texture *tex, GLint format);
int teximg_upload_rgb(struct _texture *tex);
int teximg_upload_bgr(struct _texture *tex);
int teximg_upload_bgra(struct _texture *tex);
int teximg_upload_rgba(struct _texture *tex);

void teximg_unprep_generic(struct _texture *tex, unsigned int mip);

void teximg_resample(uint8_t *p_in, int inwidth, int inheight,
			uint8_t *p_out, int outwidth, int outheight,
			GLint format);

#endif /* __TEXIMG_INTERNAL_HEADER_INCLUDED__ */
