/*
 * md5mesh.c -- md5mesh model loader + animation
 * last modification: aug. 14, 2007
 *
 * Doom3's md5mesh viewer with animation.  Mesh portion.
 * Dependences: md5model.h, md5anim.c.
 *
 * Copyright (c) 2005-2007 David HENRY
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
#include <blackbloc/vector.h>
#include <blackbloc/client.h>
#include <blackbloc/tex.h>
#include <blackbloc/img/tga.h>
#include <blackbloc/model/md5.h>

#include <math.h>
#include <stdio.h>

static struct md5_model_t md5file;
static struct md5_anim_t md5anim;

static int animated;

static struct md5_joint_t *skeleton;
static struct anim_info_t animInfo;

/* vertex array related stuff */
static int max_verts;
static int max_tris;

static vec3_t *vertexArray;
static vec2_t *texArray;
static GLuint *vertexIndices;

/**
 * Load an MD5 model from file.
 */
static int ReadMD5Model(const char *filename, struct md5_model_t *mdl)
{
	FILE *fp;
	char buff[512];
	int version;
	int curr_mesh = 0;
	unsigned int i;

	fp = fopen(filename, "rb");
	if (!fp) {
		fprintf(stderr, "Error: couldn't open \"%s\"!\n", filename);
		return 0;
	}

	while (!feof(fp)) {
		/* Read whole line */
		fgets(buff, sizeof(buff), fp);

		if (sscanf(buff, " MD5Version %d", &version) == 1) {
			if (version != 10) {
				/* Bad version */
				fprintf(stderr, "Error: bad model version\n");
				fclose(fp);
				return 0;
			}
		} else if (sscanf(buff, " numJoints %d", &mdl->num_joints) == 1) {
			if (mdl->num_joints > 0) {
				/* Allocate memory for base skeleton joints */
				mdl->baseSkel = (struct md5_joint_t *)
				    calloc(mdl->num_joints,
					   sizeof(struct md5_joint_t));
			}
		} else if (sscanf(buff, " numMeshes %d", &mdl->num_meshes) == 1) {
			if (mdl->num_meshes > 0) {
				/* Allocate memory for meshes */
				mdl->meshes = (struct md5_mesh_t *)
				    calloc(mdl->num_meshes,
					   sizeof(struct md5_mesh_t));
			}
		} else if (strncmp(buff, "joints {", 8) == 0) {
			int i;

			/* Read each joint */
			for (i = 0; i < mdl->num_joints; ++i) {
				struct md5_joint_t *joint = &mdl->baseSkel[i];

				/* Read whole line */
				fgets(buff, sizeof(buff), fp);

				if (sscanf
				    (buff, "%s %d ( %f %f %f ) ( %f %f %f )",
				     joint->name, &joint->parent,
				     &joint->pos[0], &joint->pos[1],
				     &joint->pos[2], &joint->orient[0],
				     &joint->orient[1],
				     &joint->orient[2]) == 8) {
					/* Compute the w component */
					Quat_computeW(joint->orient);
				}
			}
		} else if (strncmp(buff, "mesh {", 6) == 0) {
			struct md5_mesh_t *mesh = &mdl->meshes[curr_mesh];
			int vert_index = 0;
			int tri_index = 0;
			int weight_index = 0;
			float fdata[4];
			int idata[3];

			while ((buff[0] != '}') && !feof(fp)) {
				/* Read whole line */
				fgets(buff, sizeof(buff), fp);

				if (strstr(buff, "shader ")) {
					int quote = 0, j = 0;

					/* Copy the shader name whithout the quote marks */
					for (i = 0;
					     i < sizeof(buff) && (quote < 2);
					     ++i) {
						if (buff[i] == '\"')
							quote++;

						if ((quote == 1)
						    && (buff[i] != '\"')) {
							mesh->shader[j] =
							    buff[i];
							j++;
						}
					}
				} else
				    if (sscanf
					(buff, " numverts %d",
					 &mesh->num_verts) == 1) {
					if (mesh->num_verts > 0) {
						/* Allocate memory for vertices */
						mesh->vertices =
						    (struct md5_vertex_t *)
						    malloc(sizeof
							   (struct md5_vertex_t)
							   * mesh->num_verts);
					}

					if (mesh->num_verts > max_verts)
						max_verts = mesh->num_verts;
				} else
				    if (sscanf
					(buff, " numtris %d",
					 &mesh->num_tris) == 1) {
					if (mesh->num_tris > 0) {
						/* Allocate memory for triangles */
						mesh->triangles =
						    (struct md5_triangle_t *)
						    malloc(sizeof
							   (struct
							    md5_triangle_t) *
							   mesh->num_tris);
					}

					if (mesh->num_tris > max_tris)
						max_tris = mesh->num_tris;
				} else
				    if (sscanf
					(buff, " numweights %d",
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
					(buff, " vert %d ( %f %f ) %d %d",
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
					(buff, " tri %d %d %d %d", &tri_index,
					 &idata[0], &idata[1],
					 &idata[2]) == 4) {
					/* Copy triangle data */
					mesh->triangles[tri_index].index[0] =
					    idata[0];
					mesh->triangles[tri_index].index[1] =
					    idata[1];
					mesh->triangles[tri_index].index[2] =
					    idata[2];
				} else
				    if (sscanf
					(buff, " weight %d %d %f ( %f %f %f )",
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

	fclose(fp);

	return 1;
}

/**
 * Free resources allocated for the model.
 */
#if 0
static void FreeModel(struct md5_model_t *mdl)
{
	int i;

	if (mdl->baseSkel) {
		free(mdl->baseSkel);
		mdl->baseSkel = NULL;
	}

	if (mdl->meshes) {
		/* Free mesh data */
		for (i = 0; i < mdl->num_meshes; ++i) {
			if (mdl->meshes[i].vertices) {
				free(mdl->meshes[i].vertices);
				mdl->meshes[i].vertices = NULL;
			}

			if (mdl->meshes[i].triangles) {
				free(mdl->meshes[i].triangles);
				mdl->meshes[i].triangles = NULL;
			}

			if (mdl->meshes[i].weights) {
				free(mdl->meshes[i].weights);
				mdl->meshes[i].weights = NULL;
			}
		}

		free(mdl->meshes);
		mdl->meshes = NULL;
	}
}
#endif

/**
 * Prepare a mesh for drawing.  Compute mesh's final vertex positions
 * given a skeleton.  Put the vertices in vertex arrays.
 */
static void
PrepareMesh(const struct md5_mesh_t *mesh, const struct md5_joint_t *skeleton)
{
	int i, j, k;

	/* Setup vertex indices */
	for (k = 0, i = 0; i < mesh->num_tris; ++i) {
		for (j = 0; j < 3; ++j, ++k)
			vertexIndices[k] = mesh->triangles[i].index[j];
	}

	/* Setup vertices */
	for (i = 0; i < mesh->num_verts; ++i) {
		vec3_t finalVertex = { 0.0f, 0.0f, 0.0f };

		/* Calculate final vertex to draw with weights */
		for (j = 0; j < mesh->vertices[i].count; ++j) {
			const struct md5_weight_t *weight
			    = &mesh->weights[mesh->vertices[i].start + j];
			const struct md5_joint_t *joint
			    = &skeleton[weight->joint];

			/* Calculate transformed vertex for this weight */
			vec3_t wv;
			Quat_rotatePoint(joint->orient, weight->pos, wv);

			/* The sum of all weight->bias should be 1.0 */
			finalVertex[0] +=
			    (joint->pos[0] + wv[0]) * weight->bias;
			finalVertex[1] +=
			    (joint->pos[1] + wv[1]) * weight->bias;
			finalVertex[2] +=
			    (joint->pos[2] + wv[2]) * weight->bias;
		}

		vertexArray[i][0] = finalVertex[0];
		vertexArray[i][1] = finalVertex[1];
		vertexArray[i][2] = finalVertex[2];
		texArray[i][0] = mesh->vertices[i].st[0];
		texArray[i][1] = 1.0f - mesh->vertices[i].st[1];
	}
}

static void AllocVertexArrays(void)
{
	vertexArray = (vec3_t *) malloc(sizeof(vec3_t) * max_verts);
	texArray = malloc(sizeof(vec2_t) * max_verts);
	vertexIndices = (GLuint *) malloc(sizeof(GLuint) * max_tris * 3);
}

#if 0
static void FreeVertexArrays(void)
{
	if (vertexArray) {
		free(vertexArray);
		vertexArray = NULL;
	}

	if (vertexIndices) {
		free(vertexIndices);
		vertexIndices = NULL;
	}
}
#endif

/**
 * Draw the skeleton as lines and points (for joints).
 */
static void DrawSkeleton(const struct md5_joint_t *skeleton, int num_joints)
{
	int i;

	/* Draw each joint */
	glPointSize(5.0f);
	glColor3f(1.0f, 0.0f, 0.0f);
	glBegin(GL_POINTS);
	for (i = 0; i < num_joints; ++i)
		glVertex3fv(skeleton[i].pos);
	glEnd();
	glPointSize(1.0f);

	/* Draw each bone */
	glColor3f(0.0f, 1.0f, 0.0f);
	glBegin(GL_LINES);
	for (i = 0; i < num_joints; ++i) {
		if (skeleton[i].parent != -1) {
			glVertex3fv(skeleton[skeleton[i].parent].pos);
			glVertex3fv(skeleton[i].pos);
		}
	}
	glEnd();
}

void md5_load(const char *filename, const char *animfile)
{
	/* Load MD5 model file */
	if (!ReadMD5Model(filename, &md5file))
		exit(EXIT_FAILURE);

	AllocVertexArrays();

	/* Load MD5 animation file */
	if (animfile) {
		if (!ReadMD5Anim(animfile, &md5anim)) {
			FreeAnim(&md5anim);
		} else {
			animInfo.frame_rate = md5anim.frameRate;

			/* Allocate memory for animated skeleton */
			skeleton = (struct md5_joint_t *)
			    malloc(sizeof(struct md5_joint_t) *
				   md5anim.num_joints);

			animated = 1;
		}
	}

	if (!animated)
		printf("init: no animation loaded.\n");
}

#if 0
void cleanup(void)
{
	FreeModel(&md5file);
	FreeAnim(&md5anim);

	if (animated && skeleton) {
		free(skeleton);
		skeleton = NULL;
	}

	FreeVertexArrays();
}
#endif

static void Animate(const struct md5_anim_t *anim, struct anim_info_t *animInfo)
{
	double time, the_lerp;
	int frame, cur, next;

	time = (client_frame + lerp) / 10.0;
	frame = floor(time * animInfo->frame_rate);
	the_lerp = (time * animInfo->frame_rate) -
			floor(time * animInfo->frame_rate);

	cur = frame % anim->num_frames;
	next = (frame + 1) % anim->num_frames;

	InterpolateSkeletons(md5anim.skelFrames[cur],
			     md5anim.skelFrames[next],
			     md5anim.num_joints,
			     the_lerp,
			     skeleton);
}
void md5_render(void)
{
	int i;
	texture_t skin;

	skin = tga_get_by_name("d3/demo/models/characters/male_npc/marine/marine.tga");
	if (animated) {
		Animate(&md5anim, &animInfo);
	} else {
		/* No animation, use bind-pose skeleton */
		skeleton = md5file.baseSkel;
	}

	/* Draw skeleton */
	glRotatef(90.0, -1.0, 0.0, 0.0);

	glCullFace(GL_FRONT);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	tex_bind(skin);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	/* Draw each mesh of the model */
	for (i = 0; i < md5file.num_meshes; ++i) {
		PrepareMesh(&md5file.meshes[i], skeleton);

		glVertexPointer(3, GL_FLOAT, 0, vertexArray);
		glTexCoordPointer(2, GL_FLOAT, 0, texArray);

		glDrawElements(GL_TRIANGLES, md5file.meshes[i].num_tris * 3,
			       GL_UNSIGNED_INT, vertexIndices);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#if 0
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	DrawSkeleton(skeleton, md5file.num_joints);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_TEXTURE_2D);
#endif
	glCullFace(GL_BACK);
	glRotatef(-90.0, -1.0, 0.0, 0.0);
}
