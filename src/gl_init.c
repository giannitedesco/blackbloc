/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*
* Code responsible for initialising the OpenGL graphics context.
*/
#include <SDL.h>
#include <blackbloc.h>
#include <gl_init.h>

static SDL_Surface *screen;

/* Initialise openGL */
int gl_init(int vid_x, int vid_y, int depth, int fs)
{
	int flags=SDL_OPENGL;

	if ( fs )
		flags|=SDL_FULLSCREEN;

	/* Need 5 bits of color depth for each color */
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

	/* Enable double buffering */
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	/* Setup the depth buffer, 16 deep */
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

	/* Setup the SDL display */
	if ( !(screen=SDL_SetVideoMode(vid_x,vid_y,depth,flags)) ) {
		con_printf("gl_init: SDL_SetVideoMode: %s\n", SDL_GetError());
		return 0;
	}

	SDL_WM_SetCaption("blackbloc", "blackbloc");
	SDL_ShowCursor(0);

	/* Print out OpenGL info */
	con_printf("gl_vidmode: %i x %i x %i\n", vid_x, vid_y, depth);
	con_printf("gl_vendor: %s\n", glGetString(GL_VENDOR));
	con_printf("gl_renderer: %s\n", glGetString(GL_RENDERER));
	con_printf("gl_version: %s\n", glGetString(GL_VERSION));
	con_printf("extensions: %s\n", glGetString(GL_EXTENSIONS));

	glClearColor(0, 0, 0, 1);
	glShadeModel(GL_SMOOTH);
	glDisable(GL_LIGHTING);

	return 1;
}
