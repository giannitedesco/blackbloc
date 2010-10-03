/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __TEXIMAGE_HEADER_INCLUDED__
#define __TEXIMAGE_HEADER_INCLUDED__

typedef struct _texture *texture_t;

void tex_put(texture_t tex);
void tex_get(texture_t tex);
void tex_bind(texture_t tex);
void tex_unbind(texture_t tex);
unsigned int tex_width(texture_t tex);
unsigned int tex_height(texture_t tex);

#endif /* __TEXIMAGE_HEADER_INCLUDED__ */
