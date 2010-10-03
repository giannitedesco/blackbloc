/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*
* Stuff common to client and server.
*/
#ifndef __BLACKBLOC_HEADER_INCLUDED__
#define __BLACKBLOC_HEADER_INCLUDED__

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <blackbloc/compiler.h>

#if HAVE_STDINT_H
#include <stdint.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#if HAVE_ENDIAN_H
#include <endian.h>
#endif

/* TODO: deprecate */
#include <blackbloc/bytesex.h>

#include <blackbloc/list.h>
#include <blackbloc/vector.h>
#include <blackbloc/gfile.h>

/* Animation frame */
typedef uint16_t aframe_t;

/* Client/Server frame */
typedef uint32_t frame_t;

/* Basic immutable attributes ingrained in to the client/server
 * communations protocol, anything specific to client or server
 * goes in to cl_ent or sv_ent respectively */
struct serverstate {
	uint32_t flags;

	/* For dead reckoning */
	vector_t velocity;

	vector_t origin;
	vector_t oldorigin;

	vector_t angles;
	vector_t oldangles;

	vector_t mins, maxs;

	/* Current animation sequence */
	aframe_t start_frame;
	aframe_t anim_frames;
	aframe_t frame;

	int skinnum;

	void *model;

	struct model_ops *model_ops;
};

struct cl_ent {
	struct serverstate s;

	void *skin;

	/* For interpolation of animation */
	frame_t last_rendered;
	aframe_t oldframe;
};

typedef void *(*proc_mdl_load)(const char *);
typedef void (*proc_mdl_extents)(void *, aframe_t, vector_t, vector_t);
typedef int (*proc_mdl_numskins)(void *);
typedef void (*proc_mdl_render)(struct cl_ent *ent);
typedef void (*proc_mdl_skin)(struct cl_ent *ent, int);

struct model_ops {
	const char *type_name;
	const char *type_desc;

	/* Required by client and server */
	proc_mdl_load load;
	proc_mdl_extents extents;
	proc_mdl_numskins num_skins;

	/* Client side only */
	proc_mdl_render render;
	proc_mdl_skin skin_by_index;
};

static inline const char *get_err(void)
{
	return strerror(errno);
}

static inline const char *load_err(const char *msg)
{
	if ( errno == 0 && msg )
		return msg;
	return strerror(errno);
}

void con_printf(const char *fmt, ...);

int easy_explode(char *str, char split,
			char **toks, int max_toks);

/* Filesystem I/O API */
int game_open(struct gfile *f, const char *name);
void game_close(struct gfile *f);

#endif /* __BLACKBLOC_HEADER_INCLUDED__ */
