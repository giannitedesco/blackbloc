/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __PARTICLE_HEADER_INCLUDED__
#define __PARTICLE_HEADER_INCLUDED__

struct particle_head {
	struct particle *next, *prev;
};

struct particle {
	struct particle *next, *prev;

	/* required for rendering */
	vector_t origin;
	scalar_t radius;
	pixel_t color;

	/* virtual method table */
	void (*dtor)(struct particle *);
	void (*think)(struct particle *);
};

int particle_init(void);
void particle_render(void);
void particle_add(struct particle *);
void particle_delete(struct particle *);

#endif /* __PARTICLE_HEADER_INCLUDED__ */
