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
#include <blackbloc/img/tga.h>
#include <blackbloc/model/md5.h>

#include "md5.h"

/* OpenGL vertex array related stuff */
static unsigned int max_verts;
static unsigned int max_tris;
static vec3_t *vertexArray;
//static vec3_t *tangentArray;
static vec3_t *normalArray;
static vec2_t *texArray;
static GLuint *vertexIndices;

static int AllocVertexArrays(const struct md5_mesh *mesh)
{
	if ( mesh->max_verts > max_verts ) {
		free(vertexArray);
		vertexArray = malloc(sizeof(vec3_t) * mesh->max_verts);
		if ( NULL == vertexArray )
			goto err;

		free(normalArray);
		normalArray = malloc(sizeof(vec3_t) * mesh->max_verts);
		if ( NULL == normalArray )
			goto err;

#if 0
		free(tangentArray);
		tangentArray = malloc(sizeof(vec3_t) * mesh->max_verts);
		if ( NULL == tangentArray )
			goto err;
#endif

		free(texArray);
		texArray = malloc(sizeof(vec2_t) * mesh->max_verts);
		if ( NULL == texArray )
			goto err;
	}
	if ( mesh->max_tris > max_tris ) {
		free(vertexIndices);
		vertexIndices = malloc(sizeof(GLuint) * mesh->max_tris * 3);
		if ( NULL == vertexIndices )
			goto err;
	}

	max_verts = mesh->max_verts;
	max_tris = mesh->max_tris;
	return 1;
err:
	free(vertexArray);
//	free(tangentArray);
	free(normalArray);
	free(texArray);
	free(vertexIndices);
	max_verts = max_tris = 0;
	return 0;

}

/**
 * Prepare a mesh for drawing.  Compute mesh's final vertex positions
 * given a skeleton.  Put the vertices in vertex arrays.
 */
static void
PrepareMesh(const struct md5_mesh_part *mesh, const struct md5_joint_t *skeleton)
{
	unsigned int i, j, k;

	/* Setup vertex indices */
	for (k = 0, i = 0; i < mesh->num_tris; ++i) {
		for (j = 0; j < 3; ++j, ++k)
			vertexIndices[k] = mesh->triangles[i].index[j];
	}

	/* Setup vertices */
	for (i = 0; i < mesh->num_verts; ++i) {
		vec3_t vert = { 0.0f, 0.0f, 0.0f };
		vec3_t norm = { 0.0f, 0.0f, 0.0f };
//		vec3_t tan = { 0.0f, 0.0f, 0.0f };

		/* Calculate final vertex to draw with weights */
		for (j = 0; j < mesh->vertices[i].count; ++j) {
			const struct md5_weight_t *weight
			    = &mesh->weights[mesh->vertices[i].start + j];
			const struct md5_joint_t *joint
			    = &skeleton[weight->joint];
			vec3_t wv;

			/* Calculate transformed vertex for this weight */
			Quat_rotatePoint(joint->orient, weight->pos, wv);

			/* The sum of all weight->bias should be 1.0 */
			vert[0] +=
			    (joint->pos[0] + wv[0]) * weight->bias;
			vert[1] +=
			    (joint->pos[1] + wv[1]) * weight->bias;
			vert[2] +=
			    (joint->pos[2] + wv[2]) * weight->bias;

			Quat_rotatePoint(joint->orient, weight->normal, wv);
			v_scale(wv, weight->bias);
			v_add(norm, norm, wv);

//			Quat_rotatePoint(joint->orient, weight->tangent, wv);
//			v_scale(wv, weight->bias);
//			v_add(tan, tan, wv);
		}

		v_normalize(norm);
//		v_normalize(tan);

		vertexArray[i][0] = vert[0];
		vertexArray[i][1] = vert[1];
		vertexArray[i][2] = vert[2];

		normalArray[i][0] = norm[0];
		normalArray[i][1] = norm[1];
		normalArray[i][2] = norm[2];

#if 0
		tangentArray[i][0] = tan[0];
		tangentArray[i][1] = tan[1];
		tangentArray[i][2] = tan[2];
#endif
		texArray[i][0] = mesh->vertices[i].st[0];
		texArray[i][1] = 1.0f - mesh->vertices[i].st[1];
	}
}

/**
 * Draw the skeleton as lines and points (for joints).
 */
#if 0
static void DrawSkeleton(const struct md5_joint_t *skeleton,
				unsigned int num_joints)
{
	unsigned int i;

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
#endif

static void Animate(const struct _md5_model *model)
{
	const struct md5_anim *anim = model->anim;
	double time, the_lerp;
	int frame, cur, next;

	time = (client_frame + lerp) / 10.0;
	frame = floor(time * anim->frameRate);
	the_lerp = (time * anim->frameRate) -
			floor(time * anim->frameRate);

	cur = frame % anim->num_frames;
	next = (frame + 1) % anim->num_frames;

	InterpolateSkeletons(anim->skelFrames[cur],
				anim->skelFrames[next],
				anim->num_joints,
				the_lerp,
				model->skeleton);
}

static int compile_shader(const char *fn, GLint type, GLuint *id)
{
	struct gfile file;
	GLchar *str[1];
	GLint sz[1];
	GLint compiled;
	GLint s;

	if ( !game_open(&file, fn) )
		return 0;

	s = glCreateShader(type);

	str[0] = (GLchar *)file.f_ptr;
	sz[0] = file.f_len;
	glShaderSource(s, 1, str, sz);

	glCompileShader(s);

	glGetShaderiv(s, GL_COMPILE_STATUS, &compiled);
	if ( !compiled ) {
		GLsizei len;
		GLchar buf[512];
		glGetShaderInfoLog(s, sizeof(buf), &len, buf);
		con_printf("shader %s compile error: %.*s\n", fn, len, buf);
		exit(1);
	}

	*id = s;
	return 1;
}

static GLuint prog;
static GLuint u_Texture, u_normalTexture;
static void load_shaders(void)
{
	static int done;
	GLuint s_vert;
	GLuint s_frag;
	GLint linked;

	if ( done )
		return;

	prog = glCreateProgram();

	compile_shader("shaders/md5_vert.glsl", GL_VERTEX_SHADER, &s_vert);
	compile_shader("shaders/md5_frag.glsl", GL_FRAGMENT_SHADER, &s_frag);

	glAttachShader(prog, s_vert);
	glAttachShader(prog, s_frag);

	glLinkProgram(prog);
	glGetProgramiv(prog, GL_LINK_STATUS, &linked);
	if ( !linked ) {
		GLsizei len;
		GLchar buf[512];
		glGetProgramInfoLog(prog, sizeof(buf), &len, buf);
		con_printf("shaders link error: %.*s\n", len, buf);
		exit(1);
	}

	u_Texture = glGetUniformLocationARB(prog, "Texture");
	u_normalTexture = glGetUniformLocationARB(prog, "normalTexture");

	done = 1;
}

void md5_render(md5_model_t md5)
{
	const struct md5_mesh *mesh = md5->mesh;
	vector_t org, rot;
	unsigned int i;

	if ( !AllocVertexArrays(mesh) )
		return;

	v_copy(org, md5->ent.origin);
	v_copy(rot, md5->ent.angles);

	if (md5->anim) {
		/* TODO: interpolate motion */
		Animate(md5);
	}

	glTranslatef(org[X], org[Y], org[Z]);
	glRotatef(rot[X] - 90, 1, 0, 0);
	glRotatef(rot[Y], 0, 1, 0);
	glRotatef(rot[Z], 0, 0, 1);

	glCullFace(GL_FRONT);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	//glEnableClientState(GL_VERTEX_ATTRIB_ARRAY);
	//glEnableVertexAttribArray();
	glEnableClientState(GL_NORMAL_ARRAY);

	glUseProgram(prog);

	glUniform1iARB(u_Texture,0);
	glUniform1iARB(u_normalTexture,1);

	/* Draw each mesh of the model */
	for (i = 0; i < mesh->num_meshes; ++i) {
		PrepareMesh(&mesh->meshes[i], md5->skeleton);
		glActiveTextureARB(GL_TEXTURE0);
		tex_bind(mesh->meshes[i].skin);
		glActiveTextureARB(GL_TEXTURE1);
		tex_bind(mesh->meshes[i].normalmap);
		glActiveTextureARB(GL_TEXTURE0);

		glVertexPointer(3, GL_FLOAT, 0, vertexArray);
		glNormalPointer(GL_FLOAT, 0, normalArray);
		glTexCoordPointer(2, GL_FLOAT, 0, texArray);
//		glVertexAttribPointerARB(tangent_attrib, 3, GL_FLOAT, GL_FALSE, 0, tangentArray);

		glDrawElements(GL_TRIANGLES, mesh->meshes[i].num_tris * 3,
				GL_UNSIGNED_INT, vertexIndices);
	}

	glUseProgram(0);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#if 0
	/* Draw skeleton */
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	DrawSkeleton(skeleton, md5file.num_joints);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_TEXTURE_2D);
#endif
	glCullFace(GL_BACK);
}

md5_model_t md5_new(const char *name)
{
	struct _md5_model *md5;
	struct md5_mesh *mesh;

	load_shaders();

	mesh = md5_mesh_get_by_name(name);
	if ( NULL == mesh )
		return NULL;

	md5 = calloc(1, sizeof(*md5));
	if ( NULL == md5 )
		return NULL;

	md5->mesh = mesh;
	md5->skeleton = mesh->baseSkel;
	return md5;
}

void md5_spawn(md5_model_t md5, vector_t origin)
{
	v_copy(md5->ent.origin, origin);
	v_copy(md5->ent.oldorigin, origin);
}

void md5_animate(md5_model_t md5, const char *name)
{
	struct md5_anim *anim;
	struct md5_joint_t *skel;

	anim = md5_anim_get_by_name(name);
	if ( NULL == anim )
		return;
	
	/* Load MD5 animation file */
	if ( !CheckAnimValidity(md5->mesh, anim) ) {
		md5_anim_put(anim);
		return;
	}

	/* Allocate memory for animated skeleton */
	skel = malloc(sizeof(struct md5_joint_t) *
					anim->num_joints);
	if ( NULL == skel ) {
		md5_anim_put(anim);
		return;
	}

	if ( md5->anim ) {
		md5_anim_put(md5->anim);
		free(md5->skeleton);
	}

	md5->anim = anim;
	md5->skeleton = skel;
}

void md5_free(md5_model_t md5)
{
	if ( md5 ) {
		md5_mesh_put(md5->mesh);
		md5_anim_put(md5->anim);
		free(md5->skeleton);
		free(md5);
	}
}
