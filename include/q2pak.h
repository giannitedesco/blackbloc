/*
* This file is part of blackbloc
* Copyright (c) 2002 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __Q2PAK_HEADER_INCLUDED__
#define __Q2PAK_HEADER_INCLUDED__

#include <fnv_hash.h>

/* On disk format */
#define IDPAKHEADER (('K'<<24)|('C'<<16)|('A'<<8)|'P')

struct pak_dhdr {
	int32_t ident;
	int32_t dirofs;
	int32_t dirlen;
};

struct pak_dfile {
	char name[56];
	int32_t filepos;
	int32_t filelen;
};


/* API internal formats */
#define PAKHASH 128

struct pak_rec {
	struct pak_rec *next;
	fnv_hash_t hash;
	char *name;
	char *data;
	size_t len;
	struct pak *owner;
};

struct pak {
	int use;
	char *map;
	size_t maplen;
	int fd;
	struct pak_rec *chunk;
	int numfiles;
	struct pak *next;
	char *name;
};

/* Reference counted API */
int q2pak_open(struct gfile *f, const char *path);
void q2pak_add(char *fn);

#endif /* __Q2PAK_HEADER_INCLUDED__ */
