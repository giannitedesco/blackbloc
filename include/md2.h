/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __MD2_HEADER_INCLUDED__
#define __MD2_HEADER_INCLUDED__

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
};

struct md2_triangle {
	short index_xyz[3];
	short index_st[3];
};

struct md2_trivertx {
	unsigned char v[3];
	unsigned char lightnormalindex;
};

struct md2_aliasframe {
	float scale[3];
	float translate[3];
	char name[16];
	struct md2_trivertx verts[0];
};

struct md2_frame{
	float scale[3];
	float translate[3];
	char name[16];
	const struct md2_trivertx *verts;
};

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
};

/* Internal API */
struct md2 {
	struct image **skin;

	/* Endian converted */
	int *glcmds;
	struct md2_frame *frame;

	/* Pointers in to mapped file */
	const char *skins;

	size_t num_frames;
	size_t num_xyz;
	size_t num_glcmds;
	size_t num_skins;

	struct gfile f;
};

extern struct model_ops md2_ops;
extern struct model_ops md2_sv_ops;

void *md2_load(const char *name);
void md2_extents(struct md2 *, aframe_t, vector_t mins, vector_t maxs);
int md2_num_skins(struct md2 *);
void md2_render(struct cl_ent *);
void md2_skin(struct cl_ent *, int);

#endif /* __MD2_HEADER_INCLUDED__ */
