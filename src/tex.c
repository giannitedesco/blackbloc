/*
* This file is part of blackbloc
* Copyright (c) 2002 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*
* Texture handling routines.
*/
#include <blackbloc/blackbloc.h>
#include <blackbloc/gfile.h>
#include <blackbloc/tex.h>

#include "teximg.h"

void teximg_resample(uint8_t *p_in, int inwidth, int inheight,
			uint8_t *p_out, int outwidth, int outheight,
			GLint format)
{
	int i, j;
	unsigned *inrow, *inrow2;
	unsigned frac, fracstep;
	unsigned p1[1024], p2[1024];
	uint8_t *pix1, *pix2, *pix3, *pix4;
	unsigned *in = (unsigned *)p_in;
	unsigned *out = (unsigned *)p_out;

	fracstep = inwidth * 0x10000 / outwidth;

	frac = fracstep >> 2;
	for (i=0 ; i < outwidth ; i++)
	{
		p1[i] = 4*(frac>>16);
		frac += fracstep;
	}
	frac = 3 * (fracstep >> 2);
	for (i=0 ; i < outwidth ; i++)
	{
		p2[i] = 4 * (frac >> 16);
		frac += fracstep;
	}

	for (i=0 ; i<outheight ; i++, out += outwidth)
	{
		inrow = in + inwidth *
			(int)((i + 0.25) * inheight / outheight);
		inrow2 = in + inwidth *
			(int)((i + 0.75) * inheight / outheight);
		frac = fracstep >> 1;
		for (j=0 ; j<outwidth ; j++)
		{
			pix1 = (uint8_t *)inrow + p1[j];
			pix2 = (uint8_t *)inrow + p2[j];
			pix3 = (uint8_t *)inrow2 + p1[j];
			pix4 = (uint8_t *)inrow2 + p2[j];
			((uint8_t *)(out+j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0])>>2;
			((uint8_t *)(out+j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1])>>2;
			((uint8_t *)(out+j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2])>>2;
			((uint8_t *)(out+j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3])>>2;
		}
	}
	con_printf("resampled: %ux%u RGB -> %ux%u %s\n",
			inwidth, inheight, outwidth, outheight,
			(format == GL_RGBA) ? "RGBA" : "BGRA");
}

void tex_get(struct _texture *tex)
{
	if ( tex->t_ref == 0 ) {
		unsigned int i;
		for(tex->t_miplevels = i = 0; i < MAX_MIPLEVELS; i++) {
			if ( !(*tex->t_ops->prep)(tex, i) )
				break;
			tex->t_miplevels++;
		}
	}

	tex->t_ref++;
}

void tex_put(struct _texture *tex)
{
	if ( NULL == tex )
		return;

	assert(tex->t_ref);
	tex->t_ref--;
	if ( tex->t_ref == 0 ) {
		unsigned int i;
		glDeleteTextures(1, &tex->t_texnum);
		for(i = 0; i < tex->t_miplevels; i++) {
			if ( tex->t_ops->unprep )
				(*tex->t_ops->unprep)(tex, i);
		}
		(*tex->t_ops->dtor)(tex);
	}
}

void teximg_dtor_generic(texture_t tex)
{
	free(tex);
}

void tex_bind(struct _texture *tex)
{
	if ( !tex->t_uploaded ) {
		unsigned int i;
		glGenTextures(1, &tex->t_texnum);
		(*tex->t_ops->upload)(tex);
		for(i = 0; i < tex->t_miplevels; i++) {
			if ( tex->t_ops->unprep )
				(*tex->t_ops->unprep)(tex, i);
		}
		tex->t_uploaded = 1;
	}
	glBindTexture(GL_TEXTURE_2D, tex->t_texnum);
}

void tex_unbind(struct _texture *tex)
{
	if ( tex->t_uploaded ) {
		glDeleteTextures(1, &tex->t_texnum);
		tex->t_uploaded = 0;
	}
}

/* Just free it */
void teximg_unprep_generic(struct _texture *tex, unsigned int mip)
{
	free(tex->t_mipmap[mip].m_pixels);
	tex->t_mipmap[mip].m_pixels = NULL;
}

/* Upload the texture to the graphics chip */
int teximg_upload_generic(struct _texture *tex, GLint format)
{
	int i = 0;

	glBindTexture(GL_TEXTURE_2D, tex->t_texnum);

	glTexParameterf(GL_TEXTURE_2D,
		GL_TEXTURE_MAX_LEVEL, tex->t_miplevels);
	if ( tex->t_miplevels > 1 ) {
		glTexParameterf(GL_TEXTURE_2D,
			GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
	}else{
		glTexParameterf(GL_TEXTURE_2D,
			GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	glTexParameterf(GL_TEXTURE_2D,
		GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	for(i = 0; i < tex->t_miplevels; i++ ) { 
		glTexImage2D(GL_TEXTURE_2D, i, GL_RGBA,
			tex->t_mipmap[i].m_width,
			tex->t_mipmap[i].m_height,
			0, format, GL_UNSIGNED_BYTE,
			tex->t_mipmap[i].m_pixels);

		if ( tex->t_ops->unprep )
			(*tex->t_ops->unprep)(tex, i);
	}

	return 1;
}

int teximg_upload_rgba(struct _texture *i)
{
	return teximg_upload_generic(i, GL_RGBA);
}

int teximg_upload_rgb(struct _texture *i)
{
	return teximg_upload_generic(i, GL_RGB);
}

int teximg_upload_bgr(struct _texture *i)
{
	return teximg_upload_generic(i, GL_BGR);
}

int teximg_upload_bgra(struct _texture *i)
{
	return teximg_upload_generic(i, GL_BGRA);
}

unsigned int tex_width(texture_t tex)
{
	return (*tex->t_ops->width)(tex);
}

unsigned int tex_height(texture_t tex)
{
	return (*tex->t_ops->height)(tex);
}
