/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#include <SDL.h>

#include <blackbloc.h>
#include <sdl_mouse.h>
#include <client.h>

void sdl_mouse_event(SDL_Event *e)
{
	me.viewangles[PITCH] -= (float)e->motion.yrel;
	me.viewangles[YAW] -= (float)e->motion.xrel;
}
