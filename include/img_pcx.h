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
	char		manufacturer;
	char		version;
	char		encoding;
	char		bits_per_pixel;
	unsigned short	xmin,ymin,xmax,ymax;
	unsigned short	hres,vres;
	unsigned char	palette[48];
	char		reserved;
	char		color_planes;
	unsigned short	bytes_per_line;
	unsigned short	palette_type;
	char		filler[58];
};

/* Internal API */
int pcx_load_colormap(const char *name, unsigned char *map);
int pcx_load(const char *name);
struct image *pcx_get_by_name(const char *name);

#endif /* __IMG_PGX_HEADER_INCLUDED__ */
