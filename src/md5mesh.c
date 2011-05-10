/*
 * Doom3's md5mesh viewer with animation.  Mesh portion.
 * Dependences: md5model.h, md5anim.c.
 *
 * Copyright (c) 2005-2007 David HENRY
 * Copyright (c) 2010 Gianni Tedesco
 *  - Added texturelist support
 *  - Reimplemented animations
 *  - Implemented multiple anim and mesh loading
 *  - Ported to blackbloc file loading API
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * gcc -Wall -ansi -lGL -lGLU -lglut md5anim.c md5anim.c -o md5model
 */

#include <blackbloc/blackbloc.h>
#include <blackbloc/textreader.h>
#include <blackbloc/vector.h>
#include <blackbloc/tex.h>
#include <blackbloc/img/tga.h>
#include <blackbloc/model/md5.h>

#include "md5.h"
#include <stdio.h>

/**
 * Load an MD5 model from file.
 */
static int mesh_read(const char *filename, struct md5_mesh *mdl)
{
	struct gfile f;
	textreader_t txt;
	unsigned int version, curr_mesh = 0;
	unsigned int i, ret = 0;
	char *line;

	if ( !game_open(&f, filename) ) {
		con_printf("md5: error: couldn't open \"%s\"!\n", filename);
		return 0;
	}

	txt = textreader_new(f.f_ptr, f.f_len);
	if ( NULL == txt )
		goto out_close;

	while ( (line = textreader_gets(txt)) ) {
		if (sscanf(line, " MD5Version %d", &version) == 1) {
			if (version != 10) {
				/* Bad version */
				con_printf("Error: bad model version\n");
				goto out;
			}
		} else if (sscanf(line, " numJoints %d", &mdl->num_joints) == 1) {
			if (mdl->num_joints > 0) {
				/* Allocate memory for base skeleton joints */
				mdl->baseSkel = (struct md5_joint_t *)
				    calloc(mdl->num_joints,
					   sizeof(struct md5_joint_t));
			}
		} else if (sscanf(line, " numMeshes %d", &mdl->num_meshes) == 1) {
			if (mdl->num_meshes > 0) {
				/* Allocate memory for meshes */
				mdl->meshes = (struct md5_mesh_part *)
				    calloc(mdl->num_meshes,
					   sizeof(struct md5_mesh_part));
			}
		} else if (strncmp(line, "joints {", 8) == 0) {
			unsigned int i;

			/* Read each joint */
			for (i = 0; i < mdl->num_joints; ++i) {
				struct md5_joint_t *joint = &mdl->baseSkel[i];

				/* Read whole line */
				line = textreader_gets(txt);

				if (sscanf
				    (line, "%s %d ( %f %f %f ) ( %f %f %f )",
				     joint->name, &joint->parent,
				     &joint->pos[0], &joint->pos[1],
				     &joint->pos[2], &joint->orient[0],
				     &joint->orient[1],
				     &joint->orient[2]) == 8) {
					/* Compute the w component */
					Quat_computeW(joint->orient);
				}
			}
		} else if (strncmp(line, "mesh {", 6) == 0) {
			struct md5_mesh_part *mesh = &mdl->meshes[curr_mesh];
			int v = 0;
			int t = 0;
			int w = 0;
			float fdata[4];
			int idata[3];

			while ((line[0] != '}')) {
				char *shptr;

				/* Read whole line */
				line = textreader_gets(txt);

				if ( (shptr = strstr(line, "shader ")) ) {
					int quote = 0, j = 0;

					/* Copy the shader name whithout the quote marks */
					shptr += strlen("shader ");
					for (i = 0;
					     *shptr != '\0' && (quote < 2);
					     ++i, shptr++) {
						if (*shptr == '\"')
							quote++;

						if ((quote == 1)
						    && (*shptr != '\"')) {
							mesh->shader[j] =
								*shptr;
							j++;
						}
					}
				} else
				    if (sscanf
					(line, " numverts %d",
					 &mesh->num_verts) == 1) {
					if (mesh->num_verts > 0) {
						/* Allocate memory for vertices */
						mesh->vertices =
						    (struct md5_vertex_t *)
						    malloc(sizeof
							   (struct md5_vertex_t)
							   * mesh->num_verts);
						if ( mesh->num_verts >
							mdl->max_verts )
							mdl->max_verts =
								mesh->num_verts;
					}
				} else
				    if (sscanf
					(line, " numtris %d",
					 &mesh->num_tris) == 1) {
					if (mesh->num_tris > 0) {
						/* Allocate memory for triangles */
						mesh->triangles =
						    (struct md5_triangle_t *)
						    malloc(sizeof
							   (struct
							    md5_triangle_t) *
							   mesh->num_tris);
						if ( mesh->num_tris >
							mdl->max_tris )
							mdl->max_tris =
								mesh->num_tris;
					}
				} else
				    if (sscanf
					(line, " numweights %d",
					 &mesh->num_weights) == 1) {
					if (mesh->num_weights > 0) {
						/* Allocate memory for vertex weights */
						mesh->weights =
						    (struct md5_weight_t *)
						    malloc(sizeof
							   (struct md5_weight_t)
							   * mesh->num_weights);
					}
				} else
				    if (sscanf
					(line, " vert %d ( %f %f ) %d %d",
					 &v, &fdata[0], &fdata[1],
					 &idata[0], &idata[1]) == 5) {
					/* Copy vertex data */
					mesh->vertices[v].st[0] = fdata[0];
					mesh->vertices[v].st[1] = fdata[1];
					mesh->vertices[v].start = idata[0];
					mesh->vertices[v].count = idata[1];
				} else
				    if (sscanf
					(line, " tri %d %d %d %d", &t,
					 &idata[0], &idata[1],
					 &idata[2]) == 4) {
					/* Copy triangle data */
					mesh->triangles[t].index[0] = idata[1];
					mesh->triangles[t].index[1] = idata[2];
					mesh->triangles[t].index[2] = idata[0];
				} else
				    if (sscanf
					(line, " weight %d %d %f ( %f %f %f )",
					 &w, &idata[0], &fdata[3],
					 &fdata[0], &fdata[1],
					 &fdata[2]) == 6) {
					/* Copy vertex data */
					mesh->weights[w].joint = idata[0];
					mesh->weights[w].bias = fdata[3];
					v_zero(mesh->weights[w].normal);
					v_zero(mesh->weights[w].tangent);
					mesh->weights[w].pos[0] = fdata[0];
					mesh->weights[w].pos[1] = fdata[1];
					mesh->weights[w].pos[2] = fdata[2];
				}
			}

			curr_mesh++;
		}
	}

	ret = 1;

out:
	textreader_free(txt);
out_close:
	game_close(&f);
	return ret;
}

static void vertex_final_position(struct md5_mesh *mdl,
					struct md5_mesh_part *mesh,
					struct md5_vertex_t *vert,
					vec3_t out)
{
	unsigned int w;
	vec3_t ret;

	v_zero(ret);

	for(w = 0; w < vert->count; w++) {
		struct md5_weight_t *weight;
		struct md5_joint_t *joint;
		vec3_t disp, t;

		weight = &mesh->weights[vert->start + w];
		joint = &mdl->baseSkel[weight->joint];

		Quat_rotatePoint(joint->orient, weight->pos, disp);
		v_copy(t, joint->pos);
		v_add(t, t, disp);

		t[0] *= weight->bias;
		t[1] *= weight->bias;
		t[2] *= weight->bias;

		v_add(ret, ret, t);
	}

	v_copy(out, ret);
}

/* sort of inverse of the above function */
static void back_to_bone_space(struct md5_mesh *mdl,
					struct md5_mesh_part *mesh,
					struct md5_vertex_t *vert,
					vec3_t norm,
					vec3_t tan)
{
	unsigned int w;
	for(w = 0; w < vert->count; w++) {
		struct md5_weight_t *weight;
		struct md5_joint_t *joint;
		vec3_t jnorm;
		vec3_t jtan;

		weight = &mesh->weights[vert->start + w];
		joint = &mdl->baseSkel[weight->joint];

		Quat_rotatePoint(joint->orient, norm, jnorm);
		v_invert(jnorm);
		v_add(weight->normal, weight->normal, jnorm);

		Quat_rotatePoint(joint->orient, tan, jtan);
		v_invert(jtan);
		v_add(weight->tangent, weight->tangent, jtan);
	}
}

static int mesh_normals_and_tangents(struct md5_mesh *mdl,
					struct md5_mesh_part *mesh)
{
	unsigned int v, t, w;
	vec3_t *verts = NULL, *norms = NULL, *tangs = NULL;

	verts = calloc(mesh->num_verts, sizeof(*verts));
	norms = calloc(mesh->num_verts, sizeof(*verts));
	tangs = calloc(mesh->num_verts, sizeof(*verts));
	if ( NULL == verts || NULL == norms || NULL == tangs )
		goto err;

	/* translate model in to bind pose skeleton */
	for(v = 0; v < mesh->num_verts; v++) {
		struct md5_vertex_t *vert = mesh->vertices + v;
		vertex_final_position(mdl, mesh, vert, verts[v]);
	}

	for(t = 0; t < mesh->num_tris; t++) {
		struct md5_triangle_t *tri = mesh->triangles + t;
		vec3_t v1, v2, norm;
		vec2_t st1, st2;
		vec3_t tan;
		float co;

		/* calculate normals */
		v_sub(v1, verts[tri->index[2]], verts[tri->index[0]]);
		v_sub(v2, verts[tri->index[1]], verts[tri->index[0]]);
		v_crossproduct(norm, v1, v2);

		v_add(norms[tri->index[0]], norms[tri->index[0]], norm);
		v_add(norms[tri->index[1]], norms[tri->index[1]], norm);
		v_add(norms[tri->index[2]], norms[tri->index[2]], norm);

		/* calculate tangents */
		st1[0] = mesh->vertices[tri->index[2]].st[0] -
			mesh->vertices[tri->index[0]].st[0];
		st1[1] = mesh->vertices[tri->index[2]].st[1] -
			mesh->vertices[tri->index[0]].st[1];

		st2[0] = mesh->vertices[tri->index[1]].st[0] -
			mesh->vertices[tri->index[0]].st[0];
		st2[1] = mesh->vertices[tri->index[1]].st[1] -
			mesh->vertices[tri->index[0]].st[1];

		co = 1.0 / (st1[0] * st2[1] - st2[0] * st1[1]);

		tan[0] = co * ((v1[0] * st2[1])  + (v2[0] * -st1[1]));
		tan[1] = co * ((v1[1] * st2[1])  + (v2[1] * -st1[1]));
		tan[2] = co * ((v1[2] * st2[1])  + (v2[2] * -st1[1]));

		v_add(tangs[tri->index[0]], tangs[tri->index[0]], tan);
		v_add(tangs[tri->index[1]], tangs[tri->index[1]], tan);
		v_add(tangs[tri->index[2]], tangs[tri->index[2]], tan);
	}

	/* normalize all the tangent and normal contributions */
	for(v = 0; v < mesh->num_verts; v++) {
		v_normalize(norms[v]);
		v_normalize(tangs[v]);
	}

	/* translate back in to bone space */
	for(v = 0; v < mesh->num_verts; v++) {
		struct md5_vertex_t *vert = mesh->vertices + v;
		back_to_bone_space(mdl, mesh, vert, norms[v], tangs[v]);
	}

	/* normalize all those contributions */
	for(w = 0; w < mesh->num_weights; w++) {
		struct md5_weight_t *weight;
		weight = &mesh->weights[w];
		v_normalize(weight->normal);
		v_normalize(weight->tangent);
	}

	free(verts);
	free(norms);
	free(tangs);
	return 1;
err:
	free(verts);
	free(norms);
	free(tangs);
	return 0;
}

static int calc_normals_and_tangents(struct md5_mesh *mdl)
{
	unsigned int i;

	for(i = 0; i < mdl->num_meshes; i++) {
		struct md5_mesh_part *mesh = mdl->meshes + i;

		if ( !mesh_normals_and_tangents(mdl, mesh) )
			return 0;
	}

	return 1;
}

/**
 * Free resources allocated for the model.
 */
static void mesh_free(struct md5_mesh *mdl)
{
	unsigned int i;

	if (mdl->baseSkel) {
		free(mdl->baseSkel);
	}

	if (mdl->meshes) {
		/* Free mesh data */
		for (i = 0; i < mdl->num_meshes; ++i) {
			free(mdl->meshes[i].vertices);
			free(mdl->meshes[i].triangles);
			free(mdl->meshes[i].weights);
			tex_put(mdl->meshes[i].skin);
		}
		free(mdl->meshes);
	}

	free(mdl->name);
	list_del(&mdl->list);
	free(mdl);
}

static LIST_HEAD(md5_meshes);

static struct md5_mesh *do_mesh_load(const char *filename)
{
	struct md5_mesh *mesh;
	unsigned int i;

	mesh = calloc(1, sizeof(*mesh));
	if ( NULL == mesh )
		return NULL;

	/* Load MD5 model file */
	if (!mesh_read(filename, mesh))
		goto err;

	calc_normals_and_tangents(mesh);

	mesh->name = strdup(filename);
	if ( NULL == mesh->name )
		goto err;

	con_printf("md5: mesh loaded: %s\n", mesh->name);

	for(i = 0; i < mesh->num_meshes; i++) {
		char buf[strlen(mesh->meshes[i].shader) + 26];
		snprintf(buf, sizeof(buf), "%s.tga",
				mesh->meshes[i].shader);
		mesh->meshes[i].skin = tga_get_by_name(buf);
		snprintf(buf, sizeof(buf), "%s_local.tga",
				mesh->meshes[i].shader);
		mesh->meshes[i].normalmap = tga_get_by_name(buf);
#if 0
		snprintf(buf, sizeof(buf), "%s_s.tga",
				mesh->meshes[i].shader);
		mesh->meshes[i].specularmap = tga_get_by_name(buf);
		snprintf(buf, sizeof(buf), "%s_h.tga",
				mesh->meshes[i].shader);
		mesh->meshes[i].heightmap = tga_get_by_name(buf);
#endif
	}

	mesh->ref = 1;
	list_add_tail(&mesh->list, &md5_meshes);
	return mesh;
err:
	mesh_free(mesh);
	free(mesh->name);
	free(mesh);
	return NULL;
}

struct md5_mesh *md5_mesh_get_by_name(const char *filename)
{
	struct md5_mesh *mesh;

	list_for_each_entry(mesh, &md5_meshes, list) {
		if ( !strcmp(filename, mesh->name) ) {
			mesh->ref++;
			return mesh;
		}
	}

	return do_mesh_load(filename);
}

void md5_mesh_put(struct md5_mesh *mesh)
{
	assert(mesh->ref);
	if ( 0 == --mesh->ref ) {
		mesh_free(mesh);
	}
}
