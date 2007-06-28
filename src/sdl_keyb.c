/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#include <SDL.h>

#include <blackbloc.h>
#include <client.h>
#include <sdl_keyb.h>

/* ===== Key Binding Code ===== */
static struct cl_bind binds[KEY_MAX];

void sdl_keyb_bind(int k, int f, struct cl_cmd *c)
{
	if ( k < 0 || k >= KEY_MAX )
		return;

	binds[k].cmd = c;
	binds[k].flags = f;
}

void sdl_keyb_unbind(int k)
{
	binds[k].cmd = NULL;
}

static const char *keymap[KEY_MAX]={
	[ SDLK_a ] = "a",
	[ SDLK_b ] = "b",
	[ SDLK_c ] = "c",
	[ SDLK_d ] = "d",
	[ SDLK_e ] = "e",
	[ SDLK_f ] = "f",
	[ SDLK_g ] = "g",
	[ SDLK_h ] = "h",
	[ SDLK_i ] = "i",
	[ SDLK_j ] = "j",
	[ SDLK_k ] = "k",
	[ SDLK_l ] = "l",
	[ SDLK_m ] = "m",
	[ SDLK_n ] = "n",
	[ SDLK_o ] = "o",
	[ SDLK_p ] = "p",
	[ SDLK_q ] = "q",
	[ SDLK_r ] = "r",
	[ SDLK_s ] = "s",
	[ SDLK_t ] = "t",
	[ SDLK_u ] = "u",
	[ SDLK_v ] = "v",
	[ SDLK_w ] = "w",
	[ SDLK_x ] = "x",
	[ SDLK_y ] = "y",
	[ SDLK_z ] = "z",
	[ SDLK_SPACE ] = "space",
	[ SDLK_BACKQUOTE ] = "backquote",
};

int sdl_keyb_code(const char *n)
{
	int i;

	for(i=0; i<KEY_MAX; i++) {
		if ( !keymap[i] )
			continue;
		if ( !strcasecmp(n, keymap[i]) )
			return i;
	}

	return -1;
}

/* ===== Low Level Keyboard Input Driver ===== */
static void sdl_keyb_default(uint16_t k);
static uint16_t evq[KEY_QUEUE_LEN];
static int evq_idx;
static proc_keygrab keyfn = sdl_keyb_default;

static void sdl_keyb_default(uint16_t k)
{
	int key, state;

	key = sdl_keyb_key(k);
	state = sdl_keyb_state(k);

	if ( !binds[key].cmd )
		return;

	if ( state || binds[key].flags )
		binds[key].cmd->fn(state);
}

void sdl_keyb_ungrab(void)
{
	keyfn = sdl_keyb_default;
}

void sdl_keyb_grab(proc_keygrab fn)
{
	keyfn = fn;
}

/* Process all events on the keyboard queue */
void sdl_keyb_runq(void)
{
	int i;

	for(i=0; i<evq_idx; i++)
		keyfn(evq[i]);

	evq_idx = 0;
}

/* Generic function to put a button event on to the queue */
static inline void sdl_keyb_queue(int key, int state)
{
	evq[evq_idx++] = key | (state << KEY_STATE_BIT);
	if ( evq_idx >= KEY_QUEUE_LEN ) {
		con_printf("sdl_keyb: warn: queue full\n");
		sdl_keyb_runq();
	}
}

/* Handle mouse buttons */
void sdl_keyb_mouse(SDL_Event *e)
{
	if ( e->button.button > KEY_MOUSE_BUTTONS ) {
		con_printf("sdl_keyb: %ith mouse button not supported",
			e->button.button);
		return;
	}

	sdl_keyb_queue(SDLK_LAST+e->button.button, e->button.state);
}

/* Handle keyboard buttons */
void sdl_keyb_event(SDL_Event *e)
{
	sdl_keyb_queue(e->key.keysym.sym, e->key.state);
}
