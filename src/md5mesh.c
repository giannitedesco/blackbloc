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
static int ReadMD5Model(const char *filename, struct md5_mesh *mdl)
{
	struct gfile f;
	textreader_t txt;
	unsigned int version, curr_mesh;
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
			int vert_index = 0;
			int tri_index = 0;
			int weight_index = 0;
			float fdata[4];
			int idata[3];

			while ((line[0] != '}')) {
				/* Read whole line */
				line = textreader_gets(txt);
				char *shptr;

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
					 &vert_index, &fdata[0], &fdata[1],
					 &idata[0], &idata[1]) == 5) {
					/* Copy vertex data */
					mesh->vertices[vert_index].st[0] =
					    fdata[0];
					mesh->vertices[vert_index].st[1] =
					    fdata[1];
					mesh->vertices[vert_index].start =
					    idata[0];
					mesh->vertices[vert_index].count =
					    idata[1];
				} else
				    if (sscanf
					(line, " tri %d %d %d %d", &tri_index,
					 &idata[0], &idata[1],
					 &idata[2]) == 4) {
					/* Copy triangle data */
					mesh->triangles[tri_index].index[0] =
					    idata[1];
					mesh->triangles[tri_index].index[1] =
					    idata[2];
					mesh->triangles[tri_index].index[2] =
					    idata[0];
				} else
				    if (sscanf
					(line, " weight %d %d %f ( %f %f %f )",
					 &weight_index, &idata[0], &fdata[3],
					 &fdata[0], &fdata[1],
					 &fdata[2]) == 6) {
					/* Copy vertex data */
					mesh->weights[weight_index].joint =
					    idata[0];
					mesh->weights[weight_index].bias =
					    fdata[3];
					mesh->weights[weight_index].pos[0] =
					    fdata[0];
					mesh->weights[weight_index].pos[1] =
					    fdata[1];
					mesh->weights[weight_index].pos[2] =
					    fdata[2];
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

/**
 * Free resources allocated for the model.
 */
static void FreeModel(struct md5_mesh *mdl)
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
	if (!ReadMD5Model(filename, mesh)) {
	}

	mesh->name = strdup(filename);
	if ( NULL == mesh->name )
		goto err;

	con_printf("md5: mesh loaded: %s\n", mesh->name);

	for(i = 0; i < mesh->num_meshes; i++) {
		char buf[strlen(mesh->meshes[i].shader) + 13];
		snprintf(buf, sizeof(buf), "d3/demo/%s.tga",
				mesh->meshes[i].shader);
		mesh->meshes[i].skin = tga_get_by_name(buf);
	}

	mesh->ref = 1;
	list_add_tail(&mesh->list, &md5_meshes);
	return mesh;
err:
	FreeModel(mesh);
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
		FreeModel(mesh);
	}
}
