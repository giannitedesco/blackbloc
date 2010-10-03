/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __IMG_PCX_HEADER_INCLUDED__
#define __IMG_PCX_HEADER_INCLUDED__

typedef struct _pcx_img *pcx_img_t;

texture_t pcx_get_by_name(const char *name);
const uint8_t *pcx_get_colormap(pcx_img_t pcx, size_t *sz);

#endif /* __IMG_PCX_HEADER_INCLUDED__ */
