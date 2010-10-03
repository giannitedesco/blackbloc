/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __IMG_PGX_HEADER_INCLUDED__
#define __IMG_PGX_HEADER_INCLUDED__

/* On disk format */
struct tga {
	uint8_t		tga_identsize;
	uint8_t		tga_colormap_type;
	uint8_t		tga_image_type;

	uint16_t	tga_color_map_start;
	uint16_t	tga_color_map_len;
	uint8_t		tga_color_map_bits;

	uint16_t	tga_xstart;
	uint16_t	tga_ystart;
	uint16_t	tga_width;
	uint16_t	tga_height;
	uint8_t		tga_bpp;
	uint8_t		tga_descriptor;
} _packed;

/* Internal API */
int tga_load_colormap(const char *name, unsigned char *map);
int tga_load(const char *name);
struct image *tga_get_by_name(const char *name);

#endif /* __IMG_PGX_HEADER_INCLUDED__ */
