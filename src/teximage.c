/*
* This file is part of blackbloc
* Copyright (c) 2002 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*
* Texture handling routines.
*/
#include <blackbloc/blackbloc.h>
#include <blackbloc/gfile.h>
#include <blackbloc/teximage.h>

/* XXX: tidy up, try hyper filtering */
static void img_resample(uint8_t *p_in, int inwidth, int inheight,
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

void img_get(struct image *i)
{
	if ( i->ref == 0 ) {
		glGenTextures(1, &i->texnum);
		if ( !i->upload(i) ) {
			/* TODO: Use default texture */
		}
	}

	i->ref++;
}

void img_put(struct image *i)
{
	i->ref--;
	if ( i->ref == 0 )
		glDeleteTextures(1, &i->texnum);
}

void img_bind(struct image *i)
{
	glBindTexture(GL_TEXTURE_2D, i->texnum);
}

/* Just free it */
void img_free_unload(struct image *img)
{
	if ( img->mipmap[0].pixels )
		free(img->mipmap[0].pixels);

	img->mipmap[0].pixels=NULL;
}

/* Upload the texture to the graphics chip */
int img_upload_generic(struct image *img, GLint format)
{
	int i = 0;
	glBindTexture(GL_TEXTURE_2D, img->texnum);

	glTexParameterf(GL_TEXTURE_2D,
		GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D,
		GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	for(i = 0; i < 1; i++ ) { 
		glTexImage2D(GL_TEXTURE_2D, i, GL_RGBA,
			img->mipmap[0].width,
			img->mipmap[0].height,
			0, format, GL_UNSIGNED_BYTE,
			img->mipmap[0].pixels);
	}

	if ( img->unload )
		img->unload(img);

	return 1;
}

int img_upload_resample(struct image *i, GLint format)
{
	uint32_t w, h;
	uint8_t *buf;

	assert(format == GL_RGBA || format == GL_BGRA);

	/* Round to powers of 2 (for OpenGL) */
	for(w=1; w < i->mipmap[0].width; w<<=1);
	for(h=1; h < i->mipmap[0].height; h<<=1);

	if ( w != i->mipmap[0].width || h != i->mipmap[0].height ) {
		buf = malloc(w * h * 4);
		if ( buf == NULL ) {
			con_printf("img_upload_rgba: %s: malloc(): %s\n",
				i->name, get_err());
			return 0;
		}

		/* Actually resample the image */
		img_resample(i->mipmap[0].pixels,
				i->mipmap[0].width,
				i->mipmap[0].height,
				buf, w, h, format);

		/* Free old image */
		if ( i->unload )
			i->unload(i);
		i->unload = img_free_unload;

		/* Overwrite with scaled image */
		i->mipmap[0].width = w;
		i->mipmap[0].height = h;
		i->mipmap[0].pixels = buf;
	}

	return img_upload_generic(i, format);
}

int img_upload_rgba(struct image *i)
{
	return img_upload_generic(i, GL_RGBA);
}

int img_upload_rgb(struct image *i)
{
	return img_upload_generic(i, GL_RGB);
}

int img_upload_bgr(struct image *i)
{
	return img_upload_generic(i, GL_BGR);
}

int img_upload_bgra(struct image *i)
{
	return img_upload_generic(i, GL_BGRA);
}
