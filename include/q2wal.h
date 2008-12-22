/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __Q2WAL_HEADER_INCLUDED__
#define __Q2WAL_HEADER_INCLUDED__

/* On disk format */
#define MIPLEVELS 4
struct miptex {
	char 		name[32];
	unsigned int 	width, height;
	unsigned int	offsets[MIPLEVELS]; /* 4 mipmaps */
	char		animname[32]; /* next frame in anim */
	int		flags;
	int		contents;
	int		value;
};

/* Internal API */
int q2wal_load(const char *path);
int q2wal_load_by_name(const char *name);
struct image *q2wal_find(const char *n);
struct image *q2wal_get(const char *name);

#endif /* __Q2WAL_HEADER_INCLUDED__ */
