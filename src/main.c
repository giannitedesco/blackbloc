/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/

#include <stdlib.h>
#include <SDL.h>
#include <blackbloc.h>
#include <sdl_keyb.h>
#include <sdl_mouse.h>
#include <gl_render.h>
#include <client.h>

float fps = 30;
double lerp = 0;

int main(int argc, char **argv)
{
	SDL_Event e;
	uint32_t now, nextframe = 0, gl_frames = 0;
	uint32_t ctr;

	/* Initialise SDL */
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		con_printf("SDL_Init: %s\n", SDL_GetError());
		exit(1);
	}

	/* Cleanup SDL on exit */
	atexit(SDL_Quit);

	if ( !cl_init() ) {
		con_printf("client: cl_init failed\n");
		exit(1);
	}

	now = ctr = SDL_GetTicks();

	while( cl_alive ) {
		/* poll for client input events */
		while( SDL_PollEvent(&e) ) {
			switch ( e.type ) {
			case SDL_MOUSEMOTION:
				sdl_mouse_event(&e);
				break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				sdl_keyb_event(&e);
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				sdl_keyb_mouse(&e);
				break;
			case SDL_QUIT:
				cl_alive = 0;
				break;
			default:
				break;
			}
		}

		now = SDL_GetTicks();

		/* Run client frames */
		if ( now >= nextframe ) {
			nextframe = now + 100;
			lerp = 0;
			cl_frame();
		}else{
			lerp = 1.0f - ((float)nextframe - now)/100.0f;
			if ( lerp > 1.0 )
				lerp = 1.0;
		}

		/* Render a scene */
		gl_render();
		gl_frames++;

		/* Calculate FPS */
		if ( (gl_frames % 100) == 0 ) {
			fps = 100000.0f / (now - ctr);
			ctr = now;
		}
	}

	return 0;
}
