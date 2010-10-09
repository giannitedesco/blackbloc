/*
* This file is part of blackbloc
* Copyright (c) 20032010 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __MD5_INTERNAL_HEADER_INCLUDED__
#define __MD5_INTERNAL_HEADER_INCLUDED__

/* Joint */
struct md5_joint_t {
	char name[64];
	int parent;

	vec3_t pos;
	quat4_t orient;
};

/* Vertex */
struct md5_vertex_t {
	vec2_t st;

	int start; /* start weight */
	int count; /* weight count */
};

/* Triangle */
struct md5_triangle_t {
	int index[3];
};

/* Weight */
struct md5_weight_t {
	int joint;
	float bias;

	vec3_t pos;
};

/* Bounding box */
struct md5_bbox_t {
	vec3_t min;
	vec3_t max;
};

/* MD5 mesh */
struct md5_mesh_t {
	struct md5_vertex_t *vertices;
	struct md5_triangle_t *triangles;
	struct md5_weight_t *weights;

	int num_verts;
	int num_tris;
	int num_weights;

	char shader[256];
};

/* MD5 model structure */
struct md5_model_t {
	struct md5_joint_t *baseSkel;
	struct md5_mesh_t *meshes;

	int num_joints;
	int num_meshes;

	int animated;
	struct md5_joint_t *skeleton;

	struct list_head list;
};

/* Animation data */
struct md5_anim_t {
	int num_frames;
	int num_joints;
	int frameRate;

	struct md5_joint_t **skelFrames;
	struct md5_bbox_t *bboxes;

	struct list_head list;
};

int CheckAnimValidity (const struct md5_model_t *mdl,
					 const struct md5_anim_t *anim);
int ReadMD5Anim (const char *filename, struct md5_anim_t *anim);
void FreeAnim (struct md5_anim_t *anim);
void InterpolateSkeletons (const struct md5_joint_t *skelA,
				 const struct md5_joint_t *skelB,
				 int num_joints, float interp,
				 struct md5_joint_t *out);

#endif /* __MD5_INTERNAL_HEADER_INCLUDED__ */
