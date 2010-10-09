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
#include <blackbloc/model/md2.h>

#include "md2.h"

static LIST_HEAD(md2_meshes);

static struct md2_mesh *md2_mesh_load(const char *name)
{
	struct md2_mdl hdr;
	uint32_t *tmp;
	const uint32_t *tmp2;
	uint32_t i;
	size_t alias_len;
	const struct md2_aliasframe *f;
	struct md2_mesh *mesh;

	mesh = calloc(1, sizeof(*mesh));
	if ( mesh == NULL ) {
		con_printf("md2: %s: malloc(): %s\n", name, get_err());
		return NULL;
	}

	/* Open the model file */
	if ( !game_open(&mesh->f, name) ) {
		con_printf("md2: %s: no such file\n", name);
		goto err;
	}

	if ( sizeof(hdr) > mesh->f.f_len ) {
		con_printf("md2: %s: too small for header\n", name);
		goto err_close;
	}

	/* byteswap the header (its all ints) */
	memcpy(&hdr, mesh->f.f_ptr, sizeof(hdr));
	tmp = (uint32_t *)&hdr;
	for(i = 0; i < sizeof(hdr) / 4; i++) {
		*tmp = le_32(*tmp);
		tmp++;
	}

	/* Sanity check header fields (only check the ones we use) */
	if ( hdr.ident != IDALIASHEADER ||
		hdr.version != ALIAS_VERSION ) {
		con_printf("md2: %s: bad model\n", name);
		goto err_close;
	}

	if ( hdr.num_xyz <= 0 || hdr.num_xyz > MAX_VERTS ) {
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
	mesh->num_frames = hdr.num_frames;
	mesh->num_xyz = hdr.num_xyz;
	mesh->num_glcmds = hdr.num_glcmds;
	mesh->num_skins = hdr.num_skins;
	mesh->skins = mesh->f.f_ptr + hdr.ofs_skins;

	if ( hdr.ofs_skins + (hdr.num_skins * MAX_SKINNAME) > mesh->f.f_len ) {
		con_printf("md2: %s: too small for skins\n", name);
		goto err_close;
	}

	alias_len = hdr.num_frames *
			(sizeof(*f) + sizeof(*f->verts) * hdr.num_xyz);
	if ( hdr.ofs_frames + alias_len > mesh->f.f_len ) {
		con_printf("md2: %s: too small for frames\n", name);
		goto err_close;
	}

	/* Load the frames */
	mesh->frame = malloc(hdr.num_frames * sizeof(*mesh->frame));
	if ( mesh->frame == NULL ) {
		con_printf("md2: %s: out of memory for frames\n", name);
		goto err_close;
	}

	/* These values all need byteswapping */
	f = mesh->f.f_ptr + hdr.ofs_frames;
	for(i = 0; i < hdr.num_frames; i++) {
		mesh->frame[i].scale[0] = le_float(f->scale[1]);
		mesh->frame[i].scale[1] = le_float(f->scale[2]);
		mesh->frame[i].scale[2] = le_float(f->scale[0]);
		mesh->frame[i].translate[0] = le_float(f->translate[1]);
		mesh->frame[i].translate[1] = le_float(f->translate[2])+26;
		mesh->frame[i].translate[2] = le_float(f->translate[0]);
		memcpy(mesh->frame[i].name, f->name, sizeof(mesh->frame[i].name));
		mesh->frame[i].verts = f->verts;
		f = ((void *)f) + sizeof(*f) + sizeof(*f->verts) * hdr.num_xyz;
	}

	/* Load the glcmds */
	if ( hdr.ofs_glcmds + hdr.num_glcmds *
					sizeof(*mesh->glcmds) > mesh->f.f_len ) {
		con_printf("md2: %s: too small for glcmds\n", name);
		goto err_free_frames;
	}

	mesh->glcmds = malloc(mesh->num_glcmds * sizeof(*mesh->glcmds));
	if ( mesh->glcmds == NULL ) {
		con_printf("md2: %s: out of memory for GL commands\n", name);
		goto err_free_frames;
	}

	tmp2 = mesh->f.f_ptr + hdr.ofs_glcmds;
	for(i = 0; i < hdr.num_glcmds; i++) {
		mesh->glcmds[i] = le_32(tmp2[i]);
	}

	if ( mesh->glcmds[hdr.num_glcmds-1] != 0 ) {
		con_printf("md2: %s: last GL command is not zero\n", name);
		goto err_free_glcmds;
	}

	/* Yay, all done! */
	con_printf("md2: %s (%i frames / %i verts)\n",
		name, hdr.num_frames, hdr.num_xyz);

	mesh->ref = 1;
	list_add_tail(&mesh->list, &md2_meshes);
	return mesh;

err_free_glcmds:
	free(mesh->glcmds);
err_free_frames:
	free(mesh->frame);
err_close:
	game_close(&mesh->f);
err:
	free(mesh);
	return NULL;
}

struct md2_mesh *md2_mesh_get_by_name(const char *name)
{
	struct md2_mesh *mesh;

	list_for_each_entry(mesh, &md2_meshes, list) {
		if ( !strcmp(name, mesh->f.f_name) ) {
			assert(mesh->ref);
			mesh->ref++;
			return mesh;
		}
	}

	return md2_mesh_load(name);
}

void md2_mesh_put(struct md2_mesh *mesh)
{
	assert(mesh->ref);
	if ( --mesh->ref == 0 ) {
		free(mesh->glcmds);
		free(mesh->frame);
		game_close(&mesh->f);
		free(mesh);
	}
}
