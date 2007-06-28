/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __SKUNK_HASH_HEADER_INCLUDED__
#define __SKUNK_HASH_HEADER_INCLUDED__

/* Hash size */
#ifdef SKUNK_HASH_64
typedef u_int64_t skunk_hash_t;
#define FNV_PRIME       1099511628211ULL
#define FNV_OFFSET      14695981039346656037ULL
#else
typedef u_int32_t skunk_hash_t;
#define FNV_PRIME       16777619UL
#define FNV_OFFSET      2166136261UL
#endif

/* Generic record structure */
/* Fowler/Noll/Vo hash 
 * Its fast to compute, its obvious code, it performs well... */
static inline skunk_hash_t skunk_hash(const char *str)
{
	skunk_hash_t h=FNV_OFFSET;
	u_int8_t *data=(u_int8_t *)str;

	while(*data)
		h = (h*FNV_PRIME) ^ *data++;

	return h;
}

#endif /* __SKUNK_HASH_HEADER_INCLUDED__ */
