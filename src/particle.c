/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#include <blackbloc.h>
#include <gfile.h>
#include <teximage.h>
#include <client.h>
#include <particle.h>

/* Particle texture (circle thing) */
struct image ptexture;
static uint8_t data[32][32][4];

/* All particles in the world */
static struct particle_head particles={
	(struct particle *)&particles,
	(struct particle *)&particles,
};

/* Default particle deletion method */
void p_vdtor(struct particle *p)
{
	free(p);
}

/* Default particle think method */
void p_vthink(struct particle *p)
{
	/* nop */
}

void particle_add(struct particle *p)
{
	if ( p->dtor==NULL )
		p->dtor=p_vdtor;

	if ( p->think==NULL )
		p->think=p_vthink;

	p->next=particles.next;
	p->prev=(struct particle *)&particles;
	particles.next->prev=p;
	particles.next=p;
}

void particle_delete(struct particle *p)
{
	p->prev->next=p->next;
	p->next->prev=p->prev;
	p->prev=NULL;
	p->next=NULL;
	p->dtor(p);
}

static void particle_render_one(struct particle *p)
{
	vector_t up, right;
	vector_t up_right_scale, down_right_scale;
	vector_t VA[4];

	/* Make the particle always face us */
	v_angles(me.viewangles, NULL, up, right);
	v_scale(up, p->radius);
	v_scale(right, p->radius);
	v_add(up_right_scale, up, right);
	v_sub(down_right_scale, up, right);

	v_sub(VA[0], p->origin, down_right_scale);
	v_add(VA[1], p->origin, up_right_scale);
	v_add(VA[2], p->origin, down_right_scale);
	v_sub(VA[3], p->origin, up_right_scale);

	/* Select the color */
	glColor4ubv(p->color);

	/* Render that sucker */
	glTexCoord2f(0.0, 0.0);
	v_render(VA[0]);

	glTexCoord2f(1.0, 0.0);
	v_render(VA[1]);

	glTexCoord2f(1.0, 1.0);
	v_render(VA[2]);

	glTexCoord2f(0.0, 1.0);
	v_render(VA[3]);
}

void particle_render(void)
{
	struct particle *p=particles.next;

	glBindTexture(GL_TEXTURE_2D, ptexture.texnum);
	glDepthMask(GL_FALSE);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBegin(GL_QUADS);

	/* Render all particles */
	while( p!=(struct particle *)&particles ) {
		p=p->next;
		particle_render_one(p->prev);
	}

	/* Set alpha levels back */
	glColor4ub(0xff, 0xff, 0xff, 0xff);

	glEnd();
	glDepthMask(GL_TRUE);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

int particle_upload(struct image *i)
{
	ptexture.mipmap[0].pixels = ptexture.s_pixels;
	ptexture.mipmap[0].width = ptexture.s_width;
	ptexture.mipmap[0].height = ptexture.s_height;
	img_upload_rgba(i);
	return 0;
}

void particle_unload(struct image *i)
{
}

int particle_init(void)
{
	int x, y, dx2, dy, d, i;

	for (x = 0; x < 32; x++) {
		dx2 = x - 16;
		dx2 *= dx2;
		for (y = 0; y < 32; y++) {
			dy = y - 16;
			d = 255 - (dx2 + (dy * dy));
			if (d <= 0) {
				d = 0;
				for (i = 0; i < 3; i++)
					data[y][x][i] = 0;
			} else
				for (i = 0; i < 3; i++)
					data[y][x][i] = 255;

			data[y][x][3] = (unsigned char)d;
		}
	}

	ptexture.name="***particle***";
	ptexture.s_pixels = (uint8_t *)data;
	ptexture.s_width = 32;
	ptexture.s_height = 32;

	ptexture.upload=particle_upload;
	ptexture.unload=particle_unload;
	particle_upload(&ptexture);

	return 0;
}
