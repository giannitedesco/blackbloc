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
	unsigned int count; /* weight count */
};

/* Triangle */
struct md5_triangle_t {
	int index[3];
};

/* Weight */
struct md5_weight_t {
	int joint;
	float bias;
	vector_t normal;
	vector_t tangent;

	vec3_t pos;
};

/* Bounding box */
struct md5_bbox_t {
	vec3_t min;
	vec3_t max;
};

/* MD5 mesh */
struct md5_mesh_part {
	struct md5_vertex_t *vertices;
	struct md5_triangle_t *triangles;
	struct md5_weight_t *weights;

	unsigned int num_verts;
	unsigned int num_tris;
	unsigned int num_weights;

	char shader[256];
	texture_t skin;
	texture_t normalmap;
//	texture_t specularmap;
//	texture_t heightmap;
};

/* MD5 model structure */
struct md5_mesh {
	struct md5_joint_t *baseSkel;
	struct md5_mesh_part *meshes;

	unsigned int num_joints;
	unsigned int num_meshes;

	unsigned int max_verts;
	unsigned int max_tris;

	char *name;
	unsigned int ref;
	struct list_head list;
};

/* Animation data */
struct md5_anim {
	unsigned int num_frames;
	unsigned int num_joints;
	unsigned int frameRate;

	struct md5_joint_t **skelFrames;
	struct md5_bbox_t *bboxes;

	char *name;
	unsigned int ref;
	struct list_head list;
};

struct _md5_model {
	struct entity ent;
	struct md5_mesh *mesh;
	struct md5_anim *anim;
	struct md5_joint_t *skeleton;
};

struct md5_mesh *md5_mesh_get_by_name(const char *name);
void md5_mesh_put(struct md5_mesh *mesh);

struct md5_anim *md5_anim_get_by_name(const char *name);
void md5_anim_put(struct md5_anim *anim);

int CheckAnimValidity (const struct md5_mesh *mdl,
					 const struct md5_anim *anim);
int ReadMD5Anim (const char *filename, struct md5_anim *anim);
void FreeAnim (struct md5_anim *anim);
void InterpolateSkeletons (const struct md5_joint_t *skelA,
				 const struct md5_joint_t *skelB,
				 int num_joints, float interp,
				 struct md5_joint_t *out);

#endif /* __MD5_INTERNAL_HEADER_INCLUDED__ */
