/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __FNV_HASH_HEADER_INCLUDED__
#define __FNV_HASH_HEADER_INCLUDED__

/* Hash size */
#ifdef FNV_HASH_64
typedef uint64_t fnv_hash_t;
#define FNV_PRIME       1099511628211ULL
#define FNV_OFFSET      14695981039346656037ULL
#else
typedef uint32_t fnv_hash_t;
#define FNV_PRIME       16777619UL
#define FNV_OFFSET      2166136261UL
#endif

/* Generic record structure */
/* Fowler/Noll/Vo hash 
 * Its fast to compute, its obvious code, it performs well... */
static inline fnv_hash_t fnv_hash(const char *str)
{
	fnv_hash_t h=FNV_OFFSET;
	uint8_t *data=(uint8_t *)str;

	while(*data)
		h = (h*FNV_PRIME) ^ *data++;

	return h;
}

#endif /* __FNV_HASH_HEADER_INCLUDED__ */
