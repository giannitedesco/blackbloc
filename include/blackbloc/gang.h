/*
 * This file is part of dotscara
 * Copyright (c) 2003 Gianni Tedesco
 * Released under the terms of the GNU GPL version 2
 */
#ifndef _GANG_HEADER_INCLUDED_
#define _GANG_HEADER_INCLUDED_

/* for memset */
#include <string.h>

/** Gang memory area descriptor.
 * \ingroup g_gang
*/
struct gang_slab {
	/** Amount of data remaining in this area. */
	size_t len;
	/** Pointer to the remaining free data. */
	void *ptr;
	/** Mext memory area in the list. */
	struct gang_slab *next;
};

/** Gang memory allocator.
 * \ingroup g_gang
*/
struct gang {
	/** Size to make system memory requests. */
	size_t slab_size;
	/** Head of memory area list. */
	struct gang_slab *head;
	/** Tail of memory area list. */
	struct gang_slab *tail;
};

int _public gang_init(struct gang *m, size_t slab_size)
	_nonull(1);
void _public gang_fini(struct gang *m) _nonull(1);
void * _public gang_alloc(struct gang *m, size_t sz)
	_nonull(1) _malloc;
static inline void *gang_alloc0(struct gang *m, size_t sz)
	_nonull(1) _malloc;

/** Allocate an object initialized to zero.
 * \ingroup g_gang
 * @param m Gang allocator object to allocate from.
 * @param sz Size of object to allocate.
 *
 * Inline function which calls gang_alloc() and zero's the
 * returned memory area.
 *
 * @return pointer to allocated object, or NULL on failure
*/
static inline void *gang_alloc0(struct gang *m, size_t sz)
{
	void *ret;

	ret = gang_alloc(m, sz);
	if ( ret != NULL )
		memset(ret, 0, sz);

	return ret;
}

#endif /* _GANG_HEADER_INCLUDED_ */
