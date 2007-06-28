/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*
* For 2d glyphs on to the screen.
*
* TODO:
*  o Loads of console input corner cases need fixing
*  o Console key hardcoded to remove console
*/
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <SDL.h>

#include <blackbloc.h>
#include <client.h>
#include <gfile.h>
#include <teximage.h>
#include <sdl_keyb.h>
#include <img_pcx.h>
#include <img_png.h>
#include <hud.h>

#define HUD_CHAR 16
#define CON_CHAR 8

#define PROMPT_LEN 9
#define PROMPT "console> "

static struct image *crosshair;
static struct image *conchars;
static struct image *hudchars;
static struct image *conback;

static char lbuf[4096];
static char ibuf[256];
static char *iptr;
static float l_frames_left;

static char *hudmap;
static int con_x;
static int con_y;
static int hud_x;
static int hud_y;
static int ofs;

static int showcon=0;

static inline void hud_char(struct image *i, int sz, int x, int y, int num)
{
	int row,col;
	float frow,fcol,size;

	x *= sz;
	y *= sz;

	num&=0xff;

	/* space */
	if ( num == 0 || (num&0x7f) == 32 )
		return;

	/* World coordinates */
	row = num>>4;
	col = num&15;

	/* Texture coordinates */
	frow = row * 1.0f/16.0f;
	fcol = col * 1.0f/16.0f;
	size = 1.0f/16.0f;

	img_bind(i);

	glBegin(GL_QUADS);

	glTexCoord2f(fcol, frow);
	glVertex2f(x, y);

	glTexCoord2f(fcol + size, frow);
	glVertex2f(x+sz, y);

	glTexCoord2f(fcol + size, frow+size);
	glVertex2f(x+sz, y+sz);

	glTexCoord2f(fcol, frow+size);
	glVertex2f(x, y+sz);

	glEnd();
}

void crosshair_render()
{
	int sz=32;
	float xmin = (vid_x/2) - (sz/2);
	float xmax = (vid_x/2) + (sz/2);
	float ymin = (vid_y/2) - (sz/2);
	float ymax = (vid_y/2) + (sz/2);

	img_bind(crosshair);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(xmin, ymin);
	glTexCoord2f(1, 0);
	glVertex2f(xmax, ymin);
	glTexCoord2f(1, 1);
	glVertex2f(xmax, ymax);
	glTexCoord2f(0, 1);
	glVertex2f(xmin, ymax);
	glEnd();
}

static inline void con_input_begin(void)
{
	sdl_keyb_grab(con_input_char);
	memset(ibuf, 0, sizeof(ibuf));
	strcpy(ibuf, PROMPT);
	iptr = ibuf+PROMPT_LEN;
}

static inline void con_input_end(void)
{
	sdl_keyb_ungrab();
	iptr = NULL;
}

void hud_toggle_console(void)
{
	if ( !showcon ) {
		con_input_begin();
		showcon=1;
	}else{
		con_input_end();
		showcon=0;
	}
}

void con_render(void)
{
	int x,y;
	char chr;

	if ( !showcon )
		return;

	/* Draw console background */
	img_bind(conback);
	glEnable(GL_BLEND);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBegin(GL_QUADS);

	glColor4f(1,1,1,0.5);

	glTexCoord2f(0, 0.5);
	glVertex2f(0, 0);

	glTexCoord2f(0, 1);
	glVertex2f(0, vid_y/2);

	glTexCoord2f(1, 1);
	glVertex2f(vid_x, vid_y/2);

	glTexCoord2f(1, 0.5);
	glVertex2f(vid_x, 0);
	glEnd();

	glColor4f(1,1,1,1);

	for (y=0; y<con_y; y++) {
		for(x=0; x<con_x; x++) {
			if ( !(chr=hudmap[y*con_x + x]) )
				break;

			hud_char(conchars, CON_CHAR, x, y, chr);
		}
	}
}

static void hud_printf(int x, int y, const char *fmt, ...)
{
	va_list va;
	char buf[128];
	char *p;

	if ( !hudchars )
		return;

	va_start(va, fmt);
	vsnprintf(buf, sizeof(buf), fmt, va);

	for(p=buf; *p; p++) {
		switch ( *p ) {
		case '\n':
			y++;
			break;
		case '\b':
			break;
		default:
			hud_char(hudchars, HUD_CHAR, x++, y, *p);
		}
	}
	
	va_end(va);
}

void hud_render(void)
{
	crosshair_render();

	hud_printf(hud_x - 10, 0, "%.2f fps", fps);
	hud_printf(0, hud_y - 3, "origin=%.2f, %.2f, %.2f",
			me.origin[X], me.origin[Y], me.origin[Z]);


	/* Display last line of console */
	if ( l_frames_left ) {
		if ( l_frames_left > 10 ) {
			glColor4f(1, 1, 1, 0.5);
		}else{
			glColor4f(1, 1, 1, (l_frames_left + lerp)/11.0);
		}
		hud_printf(0, hud_y-1, lbuf);
		glColor4f(1, 1, 1, 1);
	}

	if ( iptr ) {
		hud_printf(0, hud_y-2, ibuf);
	}

	con_render();
}

void hud_think(void)
{
	if ( l_frames_left )
		l_frames_left--;
}

void con_printf(const char *fmt, ...)
{
	va_list va;
	int len;
	int i;
	static int mask=0;

	va_start(va, fmt);

	len=vsnprintf(lbuf, sizeof(lbuf), fmt, va);
	printf("%s", lbuf);

	if ( !hudmap )
		goto end;

	while ( ofs + len > con_x * con_y ) {
		memmove(hudmap, hudmap+con_x, con_x*(con_y-1));
		ofs -= con_x;
	}

	for(i=0; i<len; i++) {
		switch ( lbuf[i] ) {
		case '\b':
			if ( mask )
				mask=0;
			else
				mask=0x80;
			break;
		case '\n':
			hudmap[ofs]=0;
			ofs += con_x - (ofs % con_x);
			break;
		default:
			hudmap[ofs++] = lbuf[i] | mask;
		}
	}

	l_frames_left = 30.0f;

end:
	va_end(va);
}

int hud_init(void)
{
	con_x = vid_x / CON_CHAR;
	con_y = (vid_y / CON_CHAR) / 2;

	hud_x = vid_x / HUD_CHAR;
	hud_y = vid_y / HUD_CHAR;

	if ( !(hudmap=calloc(1,  con_x * con_y)) ) {
		perror("calloc");
		return -1;
	}

	con_printf("\bWelcome to blackbloc\b\n"
		"Copyright (c) 2003 Gianni Tedesco\n\n");

	return 0;
}

int hud_init2(void)
{
	if ( !(conback=pcx_get_by_name("pics/conback.pcx")) )
		return -1;

	if( !(conchars=pcx_get_by_name("pics/conchars.pcx")) )
		return -1;
	
	if ( !(hudchars=png_get_by_name("pics/hudchars.png")) )
		return -1;

	if ( !(crosshair=png_get_by_name("pics/crosshair.png")) )
		return -1;

	return 0;
}

void con_input_char(uint16_t k)
{
	int key = sdl_keyb_key(k);

	if ( !sdl_keyb_state(k) )
		return;

	switch ( key ) {
	case SDLK_BACKQUOTE:
	case SDLK_ESCAPE:
		showcon = 0;
		con_input_end();
		return;
	case SDLK_RETURN:
		cl_cmd_run(ibuf + PROMPT_LEN);

		/* Command may remove console */
		if ( iptr )
			con_input_begin();
		return;
	case SDLK_BACKSPACE:
		if ( iptr <= ibuf + PROMPT_LEN )
			return;
		iptr--;
		*iptr = '\0';
		return;
	}

	if ( isprint(key) ) {
		*iptr++ = key;
	}
}
