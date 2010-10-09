/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*
* QuakeII MD2 model loader.
*/
#include <blackbloc/blackbloc.h>
#include <blackbloc/client.h>
#include <blackbloc/gfile.h>
#include <blackbloc/tex.h>
#include <blackbloc/img/pcx.h>
#include <blackbloc/model/md2.h>

#include "md2.h"

static vector_t s_lerped[MAX_VERTS];

void md2_skin(md2_model_t md2, int idx)
{
	const char *str = md2->mesh->skins;

	if ( md2->skin && md2->skinnum == idx )
		return;

	str += (idx * MAX_SKINNAME);

	md2->skinnum = idx;
	md2->skin = pcx_get_by_name(str);
}

static void md2_bbox(vector_t org, vector_t rot, vector_t mins, vector_t maxs)
{
	vector_t box[8];
	vector_t a[3];
	vector_t tmp;
	int i;

	/* Build box points */
	for(i=0; i<8; i++) {
		box[i][0] = (i & 0x1) ? mins[0] : maxs[0];
		box[i][1] = (i & 0x2) ? mins[1] : maxs[1];
		box[i][2] = (i & 0x4) ? mins[2] : maxs[2];
	}

	/* Apply rotation and translations */
	v_angles(rot, a[0], a[1], a[2]);
	for(i=0; i<8; i++) {
		v_copy(tmp, box[i]);
		box[i][0] = -v_dotproduct(a[0], tmp);
		box[i][1] = v_dotproduct(a[1], tmp);
		box[i][2] = v_dotproduct(a[2], tmp);

		v_add(box[i], box[i], org);
	}
}

static void md2_extents(const struct md2_mesh *m, aframe_t frame,
			vector_t mins, vector_t maxs)
{
	vector_t tmp;
	uint32_t i, j;

	mins[X] = mins[Y] = mins[Z] = 999999.0f;
	maxs[X] = maxs[Y] = maxs[Z] = -99999.0f;

	for(i=0; i<m->num_xyz; i++) {
		struct md2_frame *f = m->frame + frame;

		tmp[X] = f->verts[i].v[1] * f->scale[0] + f->translate[0];
		tmp[Y] = f->verts[i].v[2] * f->scale[1] + f->translate[1];
		tmp[Z] = f->verts[i].v[0] * f->scale[2] + f->translate[2];

		for(j=0; j<3; j++) {
			if ( tmp[j] < mins[j] )
				mins[j] = tmp[j];
			if ( tmp[j] > maxs[j] )
				maxs[j] = tmp[j];
		}

	}
}

md2_model_t md2_new(const char *name)
{
	struct md2_mesh *mesh;
	struct _md2_model *md2;

	md2 = calloc(1, sizeof(*md2));
	if ( NULL == md2 )
		return NULL;

	mesh = md2_mesh_get_by_name(name);
	if ( NULL == mesh ) {
		free(md2);
		return NULL;
	}

	md2->mesh = mesh;

	return md2;
}

void md2_spawn(md2_model_t md2, vector_t origin)
{
	v_copy(md2->ent.origin, origin);
	v_copy(md2->ent.oldorigin, origin);
}

void md2_animate(md2_model_t md2, aframe_t begin, aframe_t end)
{
	md2->start_frame = begin;
	md2->anim_frames = end - begin;
	md2->old_frame = 0;
	md2->cur_frame = 0;

	/* Recalculate extents */
	md2_extents(md2->mesh, md2->start_frame,
				md2->ent.mins, md2->ent.maxs);

	/* don't want to interpolate between different animation
	 * sequences, otherwise we could get weird effects */
	md2->last_rendered = client_frame;
}

void md2_free(md2_model_t md2)
{
	md2_mesh_put(md2->mesh);
	tex_put(md2->skin);
	free(md2);
}

void md2_render(md2_model_t m)
{
	const struct md2_mesh *mesh = m->mesh;
	int *order = mesh->glcmds;
	int frame, old_frame;
	vector_t org, rot;
	int count;
	uint32_t i;

	/* Figure out which animation frame we are on */
	if ( m->last_rendered < client_frame && m->anim_frames ) {
		int diff = client_frame - m->last_rendered;
		m->cur_frame += diff;
		m->old_frame = m->cur_frame - 1;
		m->cur_frame %= m->anim_frames;
		m->old_frame %= m->anim_frames;

		md2_extents(mesh, m->start_frame + m->cur_frame,
				m->ent.mins, m->ent.maxs);
	}

	m->last_rendered = client_frame;
	frame = m->start_frame + m->cur_frame;
	old_frame = m->start_frame + m->old_frame;

	/* Clamp interpolation */
	if ( lerp > 1 )
		lerp = 1;

	/* Interpolate origin */
	v_zero(org);
	v_sub(org, m->ent.origin, m->ent.oldorigin);
	v_scale(org, lerp);
	v_add(org, org, m->ent.oldorigin);

	/* Interpolate angles */
	v_sub(rot, m->ent.angles, m->ent.oldangles);
	v_scale(rot, lerp);
	v_add(rot, rot, m->ent.oldangles);

	/* Copy old_frame in to buffer */
	for(i = 0; i < mesh->num_xyz; i++) {
		struct md2_frame *f = mesh->frame + old_frame;

		s_lerped[i][X] = f->verts[i].v[1] * f->scale[0] + f->translate[0];
		s_lerped[i][Y] = f->verts[i].v[2] * f->scale[1] + f->translate[1];
		s_lerped[i][Z] = f->verts[i].v[0] * f->scale[2] + f->translate[2];
	}

	/* Interpolate between that and this frame */
	for(i = 0; i < mesh->num_xyz; i++) {
		struct md2_frame *f = mesh->frame + frame;
		vector_t v;

		v[X] = s_lerped[i][X] - (f->verts[i].v[1] * f->scale[0] + f->translate[0]);
		v[Y] = s_lerped[i][Y] - (f->verts[i].v[2] * f->scale[1] + f->translate[1]);
		v[Z] = s_lerped[i][Z] - (f->verts[i].v[0] * f->scale[2] + f->translate[2]);

		v_scale(v, lerp);
		v_sub(s_lerped[i], s_lerped[i], v);
	}

	md2_bbox(org, rot, m->ent.mins, m->ent.maxs);

	glTranslatef(org[X], org[Y], org[Z]);
	glRotatef(rot[X], 1, 0, 0);
	glRotatef(rot[Y], 0, 1, 0);
	glRotatef(rot[Z], 0, 0, 1);

	/* Backwards winding order in glcmds */
	glCullFace(GL_FRONT);

	glColor4f(1.0, 1.0, 1.0, 1.0);

	tex_bind(m->skin);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	while(1) {
		count = *order++;

		if ( !count )
			break;

		if ( count < 0 ) {
			count = -count;
			glBegin(GL_TRIANGLE_FAN);
		}else{
			glBegin(GL_TRIANGLE_STRIP);
		}

		do {
			glTexCoord2f(((float *)order)[0], ((float *)order)[1]);
			i = order[2];
			v_render(s_lerped[i]);
			order+=3;
		}while(--count);

		glEnd();
	}

	glCullFace(GL_BACK);
}
