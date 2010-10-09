/*
* This file is part of blackbloc
* Copyright (c) 20032010 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __MD2_INTERNAL_HEADER_INCLUDED__
#define __MD2_INTERNAL_HEADER_INCLUDED__

/* MD2 on-disk format */

#define IDALIASHEADER	(('2'<<24)+('P'<<16)+('D'<<8)+'I')
#define ALIAS_VERSION	8
#define MAX_TRIANGLES	4096
#define MAX_VERTS	2048
#define MAX_FRAMES	512
#define MAX_MD2SKINS	32
#define MAX_SKINNAME	64

#define DTRIVERTX_V0	0
#define DTRIVERTX_V1	1
#define DTRIVERTX_V2	2
#define DTRIVERTX_LNI	3
#define DTRIVERTX_SIZE	4

struct md2_stvert {
	short s,t;
} _packed;

struct md2_triangle {
	short index_xyz[3];
	short index_st[3];
} _packed;

struct md2_trivertx {
	unsigned char v[3];
	unsigned char lightnormalindex;
}; _packed

struct md2_aliasframe {
	float scale[3];
	float translate[3];
	char name[16];
	struct md2_trivertx verts[0];
}; _packed

struct md2_frame{
	float scale[3];
	float translate[3];
	char name[16];
	const struct md2_trivertx *verts;
} _packed;

/* glcmd format: A vertex is a floating point s, a floating point t and
 * an integer vertex index.
 *
 * o A positive integer starts a tristrip command followed by that many
 *   strutures.
 * o A negative integer starts a trifan command followed by -x vertices.
 * o a zero inicates the end of the list.
*/

struct md2_mdl {
	uint32_t ident, version;
	uint32_t skinwidth, skinheight;
	uint32_t framesize; /* in bytes */

	uint32_t num_skins;
	uint32_t num_xyz;
	uint32_t num_st;
	uint32_t num_tris;
	uint32_t num_glcmds; /* dwords in command list */
	uint32_t num_frames;

	uint32_t ofs_skins;
	uint32_t ofs_st;
	uint32_t ofs_tris;
	uint32_t ofs_frames;
	uint32_t ofs_glcmds;
	uint32_t ofs_end; /* end of file (??) */
} _packed;

/* Internal API */
struct md2_mesh {
	/* singletons */
	struct gfile f;
	struct list_head list;
	unsigned int ref;

	/* Endian converted */
	int *glcmds;
	struct md2_frame *frame;

	/* Pointers in to mapped file */
	const char *skins;

	size_t num_frames;
	size_t num_xyz;
	size_t num_glcmds;
	size_t num_skins;
};

struct _md2_model {
	struct entity ent;
	struct md2_mesh *mesh;

	texture_t skin;
	int skinnum;

	aframe_t start_frame;
	aframe_t anim_frames;
	aframe_t cur_frame;
	aframe_t old_frame;
	frame_t last_rendered;
};

struct md2_mesh *md2_mesh_get_by_name(const char *name);
void md2_mesh_put(struct md2_mesh *mesh);

#endif /* __MD2_INTERNAL_HEADER_INCLUDED__ */
