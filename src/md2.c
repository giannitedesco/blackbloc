/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*
* QuakeII MD2 model loader.
*/
#include <blackbloc.h>
#include <client.h>
#include <gfile.h>
#include <md2.h>

struct model_ops md2_sv_ops={
	.load = md2_load,
	.extents = (proc_mdl_extents)md2_extents,
	.num_skins = (proc_mdl_numskins)md2_num_skins,
	.type_name = "md2",
	.type_desc = "QuakeII MD2 Model",
};

int md2_num_skins(struct md2 *m)
{
	return m->num_skins;
}

void md2_extents(struct md2 *m, aframe_t frame,
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

void *md2_load(const char *name)
{
	struct md2_mdl hdr;
	uint32_t *tmp;
	uint32_t i;
	size_t alias_len;
	struct md2_aliasframe *f;
	struct md2 *m;

	if ( !(m=calloc(1, sizeof(*m))) ) {
		con_printf("md2: %s: malloc(): %s\n", name, get_err());
		return NULL;
	}

	/* Open the model file */
	if ( !game_open(&m->f, name) ) {
		con_printf("md2: %s: no such file\n", name);
		goto err;
	}

	if ( sizeof(hdr) > m->f.f_len ) {
		con_printf("md2: %s: too small for header\n", name);
		goto err_close;
	}

	/* byteswap the header (its all ints) */
	memcpy(&hdr, m->f.f_ptr, sizeof(hdr));
	tmp = (uint32_t *)&hdr;
	for(i=0; i<sizeof(hdr)/4; i++) {
		*tmp=le_32(*tmp);
		tmp++;
	}

	/* Sanity check header fields (only check the ones we use) */
	if ( hdr.ident!=IDALIASHEADER ||
		hdr.version!=ALIAS_VERSION ) {
		con_printf("md2: %s: bad model\n", name);
		goto err_close;
	}

	if ( hdr.num_xyz <=0 || hdr.num_xyz > MAX_VERTS ) {
		con_printf("md2: %s: num_xyz=%i must be between 1 and %i\n",
			name, hdr.num_xyz, MAX_VERTS);
		goto err_close;
	}

	if ( hdr.num_skins <= 0 ) {
		con_printf("md2: %s: no skins\n", name);
		goto err_close;
	}

	if ( hdr.num_frames <= 0 ) {
		con_printf("md2: %s: no frames\n", name);
		goto err_close;
	}

	if ( hdr.num_glcmds <= 0 ) {
		con_printf("md2: %s: no glcmds\n", name);
		goto err_close;
	}

	/* Grab necessary header values */
	m->num_frames = hdr.num_frames;
	m->num_xyz = hdr.num_xyz;
	m->num_glcmds = hdr.num_glcmds;
	m->num_skins = hdr.num_skins;
	m->skins = m->f.f_ptr + hdr.ofs_skins;

	if ( hdr.ofs_skins + (hdr.num_skins * MAX_SKINNAME) > m->f.f_len ) {
		con_printf("md2: %s: too small for skins\n", name);
		goto err_close;
	}

	alias_len = hdr.num_frames * (sizeof(*f) + sizeof(*f->verts) * hdr.num_xyz);
	if ( hdr.ofs_frames + alias_len > m->f.f_len ) {
		con_printf("md2: %s: too small for frames\n", name);
		goto err_close;
	}

	/* Load the frames */
	if ( !(m->frame=malloc(hdr.num_frames * sizeof(*m->frame))) ) {
		con_printf("md2: %s: out of memory for frames\n", name);
		goto err_close;
	}

	/* These values all need byteswapping */
	f= m->f.f_ptr + hdr.ofs_frames;
	for(i=0; i<hdr.num_frames; i++) {
		m->frame[i].scale[0] = le_float(f->scale[1]);
		m->frame[i].scale[1] = le_float(f->scale[2]);
		m->frame[i].scale[2] = le_float(f->scale[0]);
		m->frame[i].translate[0] = le_float(f->translate[1]);
		m->frame[i].translate[1] = le_float(f->translate[2])+26;
		m->frame[i].translate[2] = le_float(f->translate[0]);
		memcpy(m->frame[i].name, f->name, sizeof(m->frame[i].name));
		m->frame[i].verts=f->verts;
		f = ((void *)f) + sizeof(*f) + sizeof(*f->verts) * hdr.num_xyz;
	}

	/* Load the glcmds */
	if ( hdr.ofs_glcmds + hdr.num_glcmds*sizeof(*m->glcmds) > m->f.f_len ) {
		con_printf("md2: %s: too small for glcmds\n", name);
		goto err_free_frames;
	}

	if ( !(m->glcmds=malloc(m->num_glcmds * sizeof(*m->glcmds))) ) {
		con_printf("md2: %s: out of memory for GL commands\n", name);
		goto err_free_frames;
	}

	tmp=m->f.f_ptr + hdr.ofs_glcmds;
	for(i=0; i<hdr.num_glcmds; i++) {
		m->glcmds[i]=le_32(tmp[i]);
	}

	if ( m->glcmds[hdr.num_glcmds-1] != 0 ) {
		con_printf("md2: %s: last GL command is not zero\n", name);
		goto err_free_glcmds;
	}

	/* Yay, all done! */
	con_printf("md2: %s (%i frames / %i verts)\n",
		name, hdr.num_frames, hdr.num_xyz);

	return m;

err_free_glcmds:
	free(m->glcmds);
err_free_frames:
	free(m->frame);
err_close:
	game_close(&m->f);
err:
	free(m);
	return NULL;
}
