/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __GL_RENDER_HEADER_INCLUDED__
#define __GL_RENDER_HEADER_INCLUDED__

int gl_init(unsigned int x, unsigned inty,
		unsigned int depth, unsigned int fullscreen);
void gl_render(void);
unsigned int gl_render_toggle_wireframe(void);

unsigned int gl_render_vidx(void);
unsigned int gl_render_vidy(void);

#endif /* __GL_RENDER_HEADER_INCLUDED__ */
