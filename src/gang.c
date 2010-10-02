/*
* This file is part of Firestorm NIDS
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/

#include <blackbloc.h>
#include <gang.h>

/** Initialize a gang allocator.
 * \ingroup g_gang
 * @param m An gang structure to use
 * @param slab_size default size of system allocation units
 *
 * Creates a new gang allocator descriptor with the passed values
 * set.
 *
 * @return zero on error, non-zero for success.
 */
int _public gang_init(struct gang *m, size_t slab_size)
{
	if ( slab_size == 0 )
		slab_size = 8192;
	if ( slab_size < sizeof(struct gang_slab) )
		slab_size = sizeof(struct gang_slab);

	m->slab_size = slab_size;
	m->head = NULL;
	m->tail = NULL;

	return 1;
}

/** Destroy a gang allocator.
 * \ingroup g_gang
 * @param m The gang structure to destroy.
 *
 * Frees all memory allocated by the passed gang descriptor and
 * resets all members to invalid values.
 */
void _public gang_fini(struct gang *m)
{
	struct gang_slab *s;

	for(s=m->head; s;) {
		struct gang_slab *t = s;
		s = s->next;
		free(t);
	}

	memset(m, 0, sizeof(*m));
}

/** Allocator slow path.
 * \ingroup g_gang
 * @param m The gang structure to allocate from.
 * @param sz Object size.
 *
 * Slow path for allocations which may request additional memory
 * from the system.
*/
static void *gang_alloc_slow(struct gang *m, size_t sz)
{
	size_t alloc;
	size_t overhead;
	struct gang_slab *s;
	void *ret;

	overhead = sizeof(struct gang_slab);

	alloc = overhead + sz;
	if ( alloc < m->slab_size )
		alloc = m->slab_size;

	s = malloc(alloc);
	if ( s == NULL )
		return NULL;

	ret = (void *)s + overhead;
	s->ptr = (void *)s + overhead + sz;
	s->len = alloc - (overhead + sz);
	s->next = NULL;

	if ( m->tail == NULL ) {
		m->head = s;
	}else{
		m->tail->next = s;
	}

	m->tail = s;

	return ret;
}

/** Allocate an object.
 * \ingroup g_gang
 * @param m The gang structure to allocate from.
 * @param sz Object size.
 *
 * This is the fast path for allocations. If there is memory
 * available in slabs then no call will be made to the system
 * allocator malloc/brk/mmap/HeapAlloc or whatever.
 *
 * \bug There's no way to specify any alignment requierments.
*/
void *gang_alloc(struct gang *m, size_t sz)
{
	void *ret;
	void *end;

	if ( unlikely(m->tail == NULL || m->tail->len < sz) ) {
		return gang_alloc_slow(m, sz);
	}

	ret = m->tail->ptr;
	end = m->tail->ptr + m->tail->len;
	m->tail->ptr += sz;

	if ( m->tail->ptr > end )
		m->tail->len = 0;
	else
		m->tail->len -= sz;

	return ret;
}
