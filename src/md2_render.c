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

struct model_ops md2_ops={
	.type_name = "md2",
	.type_desc = "QuakeII MD2 Model",
	.load = md2_load,
	.extents = (proc_mdl_extents)md2_extents,
	.num_skins = (proc_mdl_numskins)md2_num_skins,
	.render = md2_render,
	.skin_by_index = md2_skin,
};

static vector_t s_lerped[MAX_VERTS];

void md2_skin(struct cl_ent *ent, int idx)
{
	struct md2 *m = ent->s.model;
	const char *str = m->skins;

	str += (idx * MAX_SKINNAME);

	ent->skin = pcx_get_by_name(str);
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

void md2_render(struct cl_ent *ent)
{
	struct md2 *m = ent->s.model;
	int *order = m->glcmds;
	int frame, oldframe;
	vector_t org, rot;
	int count;
	uint32_t i;

	if ( !m )
		return;

	/* Figure out which animation frame we are on */
	if ( ent->last_rendered < client_frame && ent->s.anim_frames ) {
		int diff = client_frame - ent->last_rendered;
		ent->s.frame += diff;
		ent->oldframe = ent->s.frame - 1;
		ent->s.frame %= ent->s.anim_frames;
		ent->oldframe %= ent->s.anim_frames;

		md2_extents(m, ent->s.start_frame + ent->s.frame,
				ent->s.mins, ent->s.maxs);
	}

	ent->last_rendered = client_frame;
	frame = ent->s.start_frame + ent->s.frame;
	oldframe = ent->s.start_frame + ent->oldframe;

	/* Clamp interpolation */
	if ( lerp > 1 )
		lerp = 1;

	/* Interpolate origin */
	v_zero(org);
	v_sub(org, ent->s.origin, ent->s.oldorigin);
	v_scale(org, lerp);
	v_add(org, org, ent->s.oldorigin);

	/* Interpolate angles */
	v_sub(rot, ent->s.angles, ent->s.oldangles);
	v_scale(rot, lerp);
	v_add(rot, rot, ent->s.oldangles);

	/* Copy oldframe in to buffer */
	for(i=0; i<m->num_xyz; i++) {
		struct md2_frame *f = m->frame + oldframe;

		s_lerped[i][X] = f->verts[i].v[1] * f->scale[0] + f->translate[0];
		s_lerped[i][Y] = f->verts[i].v[2] * f->scale[1] + f->translate[1];
		s_lerped[i][Z] = f->verts[i].v[0] * f->scale[2] + f->translate[2];
	}

	/* Interpolate between that and this frame */
	for(i=0; i<m->num_xyz; i++) {
		struct md2_frame *f = m->frame + frame;
		vector_t v;

		v[X] = s_lerped[i][X] - (f->verts[i].v[1] * f->scale[0] + f->translate[0]);
		v[Y] = s_lerped[i][Y] - (f->verts[i].v[2] * f->scale[1] + f->translate[1]);
		v[Z] = s_lerped[i][Z] - (f->verts[i].v[0] * f->scale[2] + f->translate[2]);

		v_scale(v, lerp);
		v_sub(s_lerped[i], s_lerped[i], v);
	}

	md2_bbox(org, rot, ent->s.mins, ent->s.maxs);

	glTranslatef(org[X], org[Y], org[Z]);
	glRotatef(rot[X], 1, 0, 0);
	glRotatef(rot[Y], 0, 1, 0);
	glRotatef(rot[Z], 0, 0, 1);

	/* Backwards winding order in glcmds */
	glCullFace(GL_FRONT);

	tex_bind(ent->skin);
	glColor4f(1.0, 1.0, 1.0, 1.0);
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
