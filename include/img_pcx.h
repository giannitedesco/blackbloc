/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __IMG_PGX_HEADER_INCLUDED__
#define __IMG_PGX_HEADER_INCLUDED__

/* On disk format */
#define PCX_PAL 768 /* number of palette entries */
struct pcx {
	uint8_t		manufacturer;
	uint8_t		version;
	uint8_t		encoding;
	uint8_t		bits_per_pixel;
	uint16_t	xmin,ymin,xmax,ymax;
	uint16_t	hres,vres;
	uint8_t		palette[48];
	uint8_t		reserved;
	uint8_t		color_planes;
	uint16_t	bytes_per_line;
	uint16_t	palette_type;
	uint8_t		filler[58];
};

/* Internal API */
int pcx_load_colormap(const char *name, uint8_t *map);
int pcx_load(const char *name);
struct image *pcx_get_by_name(const char *name);

#endif /* __IMG_PGX_HEADER_INCLUDED__ */
