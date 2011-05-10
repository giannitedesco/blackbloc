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

#include <GL/gl.h>
#include <GL/glext.h>

#include <blackbloc/list.h>
#include <blackbloc/vector.h>
#include <blackbloc/gfile.h>

/* Animation frame */
typedef uint16_t aframe_t;

/* Client/Server frame */
typedef uint32_t frame_t;

struct entity {
	/* For dead reckoning */
	vector_t velocity;

	vector_t origin;
	vector_t oldorigin;

	vector_t angles;
	vector_t oldangles;

	vector_t mins, maxs;
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
