/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __BSP_INTERNAL_HEADER_INCLUDED__
#define __BSP_INTERNAL_HEADER_INCLUDED__

/* QuakeII BSP on-disk format */

/* lower bits are stronger, and will eat weaker brushes completely */
#define	CONTENTS_SOLID		0x01	/* an eye is never valid in a solid */
#define	CONTENTS_WINDOW		0x02	/* translucent, but not watery */
#define	CONTENTS_AUX		0x04
#define	CONTENTS_LAVA		0x08
#define	CONTENTS_SLIME		0x10
#define	CONTENTS_WATER		0x20
#define	CONTENTS_MIST		0x40
#define	LAST_VISIBLE_CONTENTS	0x40

/* remaining contents are non-visible, and don't eat brushes */
#define	CONTENTS_AREAPORTAL	0x8000
#define	CONTENTS_PLAYERCLIP	0x10000
#define	CONTENTS_MONSTERCLIP	0x20000

/* currents can be added to any other contents, and may be mixed */
#define	CONTENTS_CURRENT_0	0x40000
#define	CONTENTS_CURRENT_90	0x80000
#define	CONTENTS_CURRENT_180	0x100000
#define	CONTENTS_CURRENT_270	0x200000
#define	CONTENTS_CURRENT_UP	0x400000
#define	CONTENTS_CURRENT_DOWN	0x800000
#define	CONTENTS_ORIGIN		0x1000000	/* removed before bsping an entity */
#define	CONTENTS_MONSTER	0x2000000	/* should never be on a brush, only in game */
#define	CONTENTS_DEADMONSTER	0x4000000
#define	CONTENTS_DETAIL		0x8000000	/* brushes to be added after vis leafs */
#define	CONTENTS_TRANSLUCENT	0x10000000	/* auto set if any surface has trans */
#define	CONTENTS_LADDER		0x20000000

#define	SURF_LIGHT	0x01	/* value will hold the light strength */
#define	SURF_SLICK	0x02	/* effects game physics */
#define	SURF_SKY	0x04	/* don't draw, but add to skybox */
#define	SURF_WARP	0x08	/* turbulent water warp */
#define	SURF_TRANS33	0x10
#define	SURF_TRANS66	0x20
#define	SURF_FLOWING	0x40	/* scroll towards angle */
#define	SURF_NODRAW	0x80	/* don't bother referencing the texture */

/* Lumps */
#define	LUMP_ENTITIES		0
#define	LUMP_PLANES		1
#define	LUMP_VERTEXES		2
#define	LUMP_VISIBILITY		3
#define	LUMP_NODES		4
#define	LUMP_TEXINFO		5
#define	LUMP_FACES		6
#define	LUMP_LIGHTING		7
#define	LUMP_LEAFS		8
#define	LUMP_LEAFFACES		9
#define	LUMP_LEAFBRUSHES	10
#define	LUMP_EDGES		11
#define	LUMP_SURFEDGES		12
#define	LUMP_MODELS		13
#define	LUMP_BRUSHES		14
#define	LUMP_BRUSHSIDES		15
#define	LUMP_POP		16
#define	LUMP_AREAS		17
#define	LUMP_AREAPORTALS	18
#define	HEADER_LUMPS		19

struct bsp_lump {
	uint32_t ofs, len;
};

#define IDBSPHEADER	(('P'<<24)+('S'<<16)+('B'<<8)+'I')
#define BSPVERSION	38
struct bsp_header {
	int32_t ident;
	int32_t version;
	struct bsp_lump lumps[HEADER_LUMPS];
};

struct bsp_model {
	float mins[3],maxs[3];
	float origin[3]; /* for sounds or lights */
	int32_t headnode;
	int32_t firstface, numfaces;
};

struct bsp_vertex {
	float point[3];
};

#define PLANE_X		0
#define PLANE_Y		1
#define PLANE_Z		2
#define PLANE_ANYX	3
#define PLANE_ANYY	4
#define PLANE_ANYZ	5

struct bsp_plane {
	float normal[3];
	float dist;
	int32_t type;
};

/* TODO: CONTENTS + SURF flags */

struct bsp_node {
	int32_t planenum;
	int32_t children[2];
	int16_t mins[3];
	int16_t maxs[3];
	uint16_t firstface;
	uint16_t numfaces; /* counting both sides */
};

struct bsp_texinfo {
	float vecs[2][4]; /* [s/t][xyz offset] */
	int32_t flags; /* miptex flags + overrides */
	int32_t value; /* light emission, etc. */
	char texture[32];
	int32_t nexttexinfo; /* for animations, -1 = end of chain */
};

/* note that edge 0 is never used, because negative edge nums are used
 * for counterclockwise use of the edge in a face
*/
struct bsp_edge {
	uint16_t v[2]; /* vertex numbers */
};

#define MAXLIGHTMAPS 4
struct bsp_face {
	uint16_t planenum;
	int16_t side;
	int32_t firstedge;
	int16_t numedges;
	int16_t texinfo;

	int8_t styles[MAXLIGHTMAPS];
	int32_t lightofs; /* start of [numstyles*surfsize] samples */
};

struct bsp_leaf {
	int32_t contents; /* OR of all brushes */
	int16_t cluster;
	int16_t area;
	int16_t mins[3];
	int16_t maxs[3];
	uint16_t firstleafface;
	uint16_t numleaffaces;
	uint16_t firstleafbrush;
	uint16_t numleafbrushes;
};

struct bsp_brushside {
	uint16_t planenum;
	int16_t texinfo;
};

struct bsp_brush {
	int32_t firstside;
	int32_t numsides;
	int32_t contents;
};

#define ANGLE_UP 	-1
#define ANGLE_DOWN 	-2

/* The visibility lump consists of a header with a count, then
 * byte offsets for the PVS and PHS of each cluster, then the raw
 * compressed bit vectors
*/
#define DVIS_PVS 0
#define DVIS_PHS 1
struct bsp_vis {
	uint32_t numclusters;
	uint32_t bitofs[8][2];
};

/* each area has a list of portals that lead in to other areas
 * when portals are closed, other areas may not be visible or
 * hearable even if the vis info says that it should be
*/
struct bsp_areaportal{
	int32_t portalnum;
	int32_t otherarea;
};

struct bsp_area {
	int32_t numareaportals;
	int32_t firstareaportal;
};

/* In-memory representation */
#define SIDE_FRONT 0
#define SIDE_BACK 1
#define SIDE_ON 2

#define SURF_PLANEBACK 2
#define SURF_DRAWSKY 4
#define SURF_DRAWTURB 0x10
#define SURF_DRAWBACKGROUND 0x40
#define SURF_UNDERWATER 0x80

#define VERTEXSIZE 7 /* x y z s t u v*/
struct bsp_poly {
	struct bsp_poly *next;
	struct bsp_poly *chain;
	int numverts;
	int flags; /* needed? */
	float verts[0][VERTEXSIZE];
};

struct bsp_mtexinfo {
	float vecs[2][4];
	int flags;
	int numframes;
	struct bsp_mtexinfo *next;
	texture_t image;
};

struct bsp_medge {
	unsigned short v[2];
	unsigned int cachededgeoffset;
};

struct bsp_mplane {
	vector_t normal;
	float dist;
	unsigned char type; /* for fast side tests */
	unsigned char signbits; /* signx + (signy<<1) + (signz<<1) */
	unsigned char pad[2]; /* wtf */
};

struct bsp_msurface {
	int visframe; /* should be drawn when node is crossed */
	struct bsp_mplane *plane;
	int flags;
	int firstedge;
	int numedges;
	short texturemins[2];
	short extents[2];
	int light_s, light_t; /* gl lightmap coords */
	int dlight_s, dlight_t; /* dynamic lights */

	struct bsp_poly *polys;

	struct bsp_msurface *texturechain;
	struct bsp_msurface *lightmapchain;

	struct bsp_mtexinfo *texinfo; /* texture */

	int dlightframe;
	int dlightbits;
	int lightmaptexturenum;
	unsigned char styles[MAXLIGHTMAPS];
	float cached_light[MAXLIGHTMAPS];
	unsigned char *samples; /* [numstyles*surfsize] */
};

struct bsp_mnode {
	int contents;
	int visframe;
	vector_t mins;
	vector_t maxs;
	struct bsp_mnode *parent;

	struct bsp_mplane *plane;
	struct bsp_mnode *children[2];
	unsigned short firstsurface;
	unsigned short numsurfaces;
};

struct bsp_mleaf {
	int contents; /* will be negative */
	int visframe; /* node needs to be traversed if current */
	vector_t mins;
	vector_t maxs;
	struct bsp_mnode *parent;

	int cluster;
	int area;
	struct bsp_msurface **firstmarksurface;
	int nummarksurfaces;
};

#define BLOCK_WIDTH 128
#define BLOCK_HEIGHT 128
#define LIGHTMAP_BYTES 4
#define GL_LIGHTMAP_FORMAT GL_RGBA

struct _q2bsp {
	struct bsp_mtexinfo *mtex;
	struct bsp_vertex *mvert;
	struct bsp_medge *medge;
	struct bsp_mplane *mplane;
	struct bsp_msurface *msurface;
	struct bsp_msurface **marksurface;
	struct bsp_mleaf *mleaf;
	struct bsp_mnode *mnode;
	struct bsp_vis *visofs;
	unsigned int num_mtex;
	int *msurfedge;
	int nummarksurfaces;
	int numsurfaces;
	int numsurfedges;
	int numplanes;
	int numleafs;
	int numnodes;
	int numedges;
	const unsigned char *map_visibility;
	const unsigned char *lightdata;
	unsigned char *vis;

	int current_lightmap_texture;
	unsigned char lightmap_buffer[4*BLOCK_WIDTH*BLOCK_HEIGHT];
	int allocated[BLOCK_WIDTH];
	unsigned int lm_textures;
	float s_blocklights[34*34*3];
};

#endif /* __BSP_INTERNAL_HEADER_INCLUDED__ */
