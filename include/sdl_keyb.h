/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __SDL_KEYB_HEADER_INCLUDED__
#define __SDL_KEYB_HEADER_INCLUDED__

#define KEY_STATE_MASK 0x8000
#define KEY_STATE_BIT 15

#define KEY_QUEUE_LEN 128

#define KEY_MOUSE_BUTTONS 10

#define KEY_MAX (SDLK_LAST + KEY_MOUSE_BUTTONS)

typedef void (*proc_keygrab)(u_int16_t);

void sdl_keyb_mouse(SDL_Event *);
void sdl_keyb_event(SDL_Event *);
void sdl_keyb_runq(void);
void sdl_keyb_bind();
void sdl_keyb_grab(proc_keygrab);
void sdl_keyb_ungrab(void);

static inline int sdl_keyb_key(u_int16_t key)
{
	return (key & ~KEY_STATE_MASK);
}
static inline int sdl_keyb_state(u_int16_t key)
{
	return (key >> KEY_STATE_BIT);
}

#endif /* __SDL_KEYB_HEADER_INCLUDED__ */
