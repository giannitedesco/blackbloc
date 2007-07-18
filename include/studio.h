/***
*
*	Copyright (c) 1998, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
****/




#ifndef _STUDIO_H_
#define _STUDIO_H_

/*
==============================================================================

STUDIO MODELS

Studio models are position independent, so the cache manager can move them.
==============================================================================
*/
#define STUDIO_IDST		('I'|'D'<<8|'S'<<16|('T'<<24))
#define STUDIO_IDSQ		('I'|'D'<<8|'S'<<16|('Q'<<24))

#define MAXSTUDIOTRIANGLES	20000	// TODO: tune this
#define MAXSTUDIOVERTS		2048	// TODO: tune this
#define MAXSTUDIOSEQUENCES	256	// total animation sequences
#define MAXSTUDIOSKINS		100	// total textures
#define MAXSTUDIOSRCBONES	512	// bones allowed at source movement
#define MAXSTUDIOBONES		128	// total bones actually used
#define MAXSTUDIOMODELS		32	// sub-models per model
#define MAXSTUDIOBODYPARTS	32
#define MAXSTUDIOGROUPS		4
#define MAXSTUDIOANIMATIONS	512	// per sequence
#define MAXSTUDIOMESHES		256
#define MAXSTUDIOEVENTS		1024
#define MAXSTUDIOPIVOTS		256
#define MAXSTUDIOCONTROLLERS 8

struct studio_hdr {
	uint32_t 	id;
	uint32_t 	version;
	uint8_t 	name[64];
	uint32_t 	length;
	vec3_t		eyeposition;	// ideal eye position
	vec3_t		min;		// ideal movement hull size
	vec3_t		max;			
	vec3_t		bbmin;		// clipping bounding box
	vec3_t		bbmax;		

	uint32_t	flags;
	uint32_t	numbones;	// bones
	uint32_t	boneindex;

	uint32_t	numbonecontrollers;	// bone controllers
	uint32_t	bonecontrollerindex;

	uint32_t	numhitboxes;	// complex bounding boxes
	uint32_t	hitboxindex;			
	
	uint32_t	numseq;		// animation sequences
	uint32_t	seqindex;

	uint32_t	numseqgroups;	// demand loaded sequences
	uint32_t	seqgroupindex;

	uint32_t	numtextures;	// raw textures
	uint32_t	textureindex;
	uint32_t	texturedataindex;

	uint32_t	numskinref;	// replaceable textures
	uint32_t	numskinfamilies;
	uint32_t	skinindex;

	uint32_t	numbodyparts;		
	uint32_t	bodypartindex;

	uint32_t	numattachments;	// queryable attachable points
	uint32_t	attachmentindex;

	uint32_t	soundtable;
	uint32_t	soundindex;
	uint32_t	soundgroups;
	uint32_t	soundgroupindex;

	uint32_t	numtransitions;	// animation node to animation node transition graph
	uint32_t	transitionindex;
};

// header for demand loaded sequence group data
struct studio_seqhdr {
	int32_t	id;
	int32_t	version;

	uint8_t name[64];
	int32_t	length;
};

// bones
struct studio_bone {
	uint8_t name[32];	// bone name for symbolic links
	uint32_t	parent;		// parent bone
	uint32_t	flags;		// ??
	int32_t		bonecontroller[6]; // bone controller index, -1 == none
	float		value[6];	// default DoF values
	float		scale[6];   // scale for delta DoF values
};


// bone controllers
struct studio_bonecontroller {
	int32_t	bone;	// -1 == 0
	int32_t	type;	// X, Y, Z, XR, YR, ZR, M
	float	start;
	float	end;
	int32_t	rest;	// char index value at rest
	uint32_t	index;	// 0-3 user set controller, 4 mouth
};

// intersection boxes
struct studio_bbox {
	uint32_t	bone;
	uint32_t	group;			// intersection group
	vec3_t	bbmin;		// bounding box
	vec3_t	bbmax;		
};

// demand loaded sequence groups
struct studio_seqgroup {
	uint8_t 	label[32];	// textual name
	uint8_t 	name[64];	// file name
	void 		*cache;		// cache index pointer
	int32_t		data;		// hack for group 0
};

// sequence descriptions
struct studio_seqdesc {
	uint8_t	label[32];	// sequence label

	float	fps;		// frames per second	
	uint32_t	flags;		// looping/non-looping flags

	int32_t	activity;
	int32_t	actweight;

	int32_t	numevents;
	int32_t	eventindex;

	int32_t	numframes;	// number of frames per sequence

	int32_t	numpivots;	// number of foot pivots
	int32_t	pivotindex;

	int32_t	motiontype;	
	int32_t	motionbone;
	vec3_t	linearmovement;
	int32_t	automoveposindex;
	int32_t	automoveangleindex;

	vec3_t	bbmin;		// per sequence bounding box
	vec3_t	bbmax;		

	int32_t	numblends;
	int32_t	animindex;		// mstudioanim_t pointer relative to start of sequence group data
										// [blend][bone][X, Y, Z, XR, YR, ZR]

	int32_t	blendtype[2];	// X, Y, Z, XR, YR, ZR
	float	blendstart[2];	// starting value
	float	blendend[2];	// ending value
	int32_t	blendparent;

	uint32_t	seqgroup;		// sequence group for demand loading

	int32_t	entrynode;		// transition node at entry
	int32_t	exitnode;		// transition node at exit
	int32_t	nodeflags;		// transition rules
	
	int32_t	nextseq;		// auto advancing sequences
};

// events
struct studio_event {
	int32_t frame;
	int32_t	event;
	int32_t	type;
	uint8_t	options[64];
};


// pivots
struct studio_pivot {
	vec3_t	org;	// pivot point
	int32_t	start;
	int32_t	end;
};

// attachment
struct studio_attachment {
	uint8_t	name[32];
	uint32_t	type;
	int32_t	bone;
	vec3_t	org;	// attachment point
	vec3_t	vectors[3];
};

struct studio_anim {
	uint16_t offset[6];
};

// animation frames
struct studio_animvalue {
	union {
		struct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
			uint8_t	valid;
			uint8_t	total;
#elif __BYTE_ORDER == __BIG_ENDIAN
			uint8_t	total;
			uint8_t	valid;
#else
#error Endian?!
#endif
		}num;
		uint16_t value;
	};
};



// body part index
struct studio_bodypart {
	uint8_t		name[64];
	uint32_t		nummodels;
	uint32_t		base;
	uint32_t		modelindex; // index into models array
};

// skin info
struct studio_texture {
	uint8_t		name[64];
	int32_t		flags;
	int32_t		width;
	int32_t		height;
	int32_t		index;
};

// skin families
// short	index[skinfamilies][skinref]

// studio models
struct studio_dmodel {
	uint8_t		name[64];

	uint32_t	type;

	float		boundingradius;

	uint32_t	nummesh;
	uint32_t	meshindex;

	uint32_t	numverts;		// number of unique vertices
	uint32_t	vertinfoindex;	// vertex bone info
	uint32_t	vertindex;		// vertex vec3_t
	uint32_t	numnorms;		// number of unique surface normals
	uint32_t	norminfoindex;	// normal bone info
	uint32_t	normindex;		// normal vec3_t

	uint32_t	numgroups;		// deformation groups
	uint32_t	groupindex;
};

// vec3_t	boundingbox[model][bone][2];	// complex intersection info

// meshes
struct studio_mesh {
	int32_t		numtris;
	int32_t		triindex;
	int32_t		skinref;
	int32_t		numnorms;		// per mesh normals
	int32_t		normindex;		// normal vec3_t
};

// triangles
#if 0
typedef struct 
{
	short				vertindex;		// index into vertex array
	short				normindex;		// index into normal array
	short				s,t;			// s,t position on skin
} mstudiotrivert_t;
#endif

// lighting options
#define STUDIO_NF_FLATSHADE	0x0001
#define STUDIO_NF_CHROME	0x0002
#define STUDIO_NF_FULLBRIGHT	0x0004

// motion flags
#define STUDIO_X		0x0001
#define STUDIO_Y		0x0002	
#define STUDIO_Z		0x0004
#define STUDIO_XR		0x0008
#define STUDIO_YR		0x0010
#define STUDIO_ZR		0x0020
#define STUDIO_LX		0x0040
#define STUDIO_LY		0x0080
#define STUDIO_LZ		0x0100
#define STUDIO_AX		0x0200
#define STUDIO_AY		0x0400
#define STUDIO_AZ		0x0800
#define STUDIO_AXR		0x1000
#define STUDIO_AYR		0x2000
#define STUDIO_AZR		0x4000
#define STUDIO_TYPES		0x7FFF
#define STUDIO_RLOOP		0x8000	// controller that wraps shortest distance

// sequence flags
#define STUDIO_LOOPING		0x0001

// bone flags
#define STUDIO_HAS_NORMALS	0x0001
#define STUDIO_HAS_VERTICES 	0x0002
#define STUDIO_HAS_BBOX		0x0004
#define STUDIO_HAS_CHROME	0x0008	// if any of the textures have chrome on them

#define RAD_TO_STUDIO		(32768.0/M_PI)
#define STUDIO_TO_RAD		(M_PI/32768.0)

#endif
