/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#include <unistd.h>
#include <SDL.h>

#include <blackbloc.h>
#include <client.h>
#include <gl_render.h>
#include <sdl_keyb.h>
#include <hud.h>
#include <gfile.h>
#include <md2.h>
#include <q2bsp.h>
#include <md5model.h>

frame_t client_frame;
int cl_alive = 1;
struct playerstate me={
	.viewoffset = {0, 40, 0},
};

static struct cl_ent ent;
static gfs_t gfs;

int game_open(struct gfile *f, const char *name)
{
	return gfile_open(gfs, f, name);
}

void game_close(struct gfile *f)
{
	gfile_close(gfs, f);
}

int cl_init(void)
{
	gfs = gfs_open("data/game.gfs");
	if ( gfs == NULL )
		return 0;

	client_frame = 0;

	/* Bind keys */
	cl_cmd_bind("backquote", "console");
	cl_cmd_bind("q", "quit");
	cl_cmd_bind("w", "+forwards");
	cl_cmd_bind("s", "+backwards");
	cl_cmd_bind("a", "+strafe_left");
	cl_cmd_bind("d", "+strafe_right");
	cl_cmd_bind("space", "+jump");
	cl_cmd_bind("c", "+crouch");

	if ( !gl_init(1920, 1080, 24, 1) )
	//if ( !gl_init(1024, 768, 24, 0) )
		return 0;

	if ( !hud_init() )
		return 0;

	ent.s.model_ops = &md2_ops;
	ent.s.origin[X] = 500;
	ent.s.origin[Y] = 450;
	ent.s.origin[Z] = 1200;
	v_copy(ent.s.oldorigin, ent.s.origin);
	cl_model_load(&ent, "models/monsters/soldier/tris.md2");
	cl_model_skin(&ent, 0);
	cl_model_animate(&ent, 146, 214);

	q2bsp_load("maps/q2dm1.bsp");

	md5_load("d3/demo/models/md5/monsters/zombies/labcoatzombie.md5mesh",
		"d3/demo/models/md5/monsters/zombies/zwalk1.md5anim");
	return 1;
}

void cl_render(void)
{
	q2bsp_render();
	//md5_render();
	cl_model_render(&ent);
}

void cl_frame(void)
{
	/* Set the frame number */
	client_frame++;

	/* Process event queue */
	sdl_keyb_runq();

	hud_think();

	/* Calculate player movement */
	cl_move();
}
