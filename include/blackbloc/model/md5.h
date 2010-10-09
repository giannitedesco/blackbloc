/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __MD5_HEADER_INCLUDED__
#define __MD5_HEADER_INCLUDED__

typedef struct _md5_model *md5_model_t;

md5_model_t md5_new(const char *name);
void md5_spawn(md5_model_t md5, vector_t origin);
void md5_animate(md5_model_t md5, const char *name);
void md5_render(md5_model_t md5);
void md5_free(md5_model_t md5);

#endif /* __MD5_HEADER_INCLUDED__ */
