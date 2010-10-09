/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __MD2_HEADER_INCLUDED__
#define __MD2_HEADER_INCLUDED__

typedef struct _md2_model *md2_model_t;

md2_model_t md2_new(const char *name);
void md2_spawn(md2_model_t md2, vector_t origin);
void md2_animate(md2_model_t md2, aframe_t begin, aframe_t end);
void md2_skin(md2_model_t md2, int skinnum);
void md2_render(md2_model_t md2);
void md2_free(md2_model_t md2);

#endif /* __MD2_HEADER_INCLUDED__ */
