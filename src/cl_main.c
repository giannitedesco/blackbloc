/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#include <unistd.h>
#include <SDL.h>

#include <blackbloc/blackbloc.h>
#include <blackbloc/client.h>
#include <blackbloc/gl_render.h>
#include <blackbloc/sdl_keyb.h>
#include <blackbloc/hud.h>
#include <blackbloc/gfile.h>
#include <blackbloc/model/md2.h>
#include <blackbloc/model/md5.h>
#include <blackbloc/map/q2bsp.h>

frame_t client_frame;
int cl_alive = 1;
struct playerstate me={
	.viewoffset = {0, 40, 0},
};

static q2bsp_t map;

static md2_model_t soldier;
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

	//if ( !gl_init(1920, 1080, 24, 1) )
	if ( !gl_init(1280, 720, 24, 0) )
		return 0;

	if ( !hud_init() )
		return 0;

	soldier = md2_new("models/monsters/soldier/tris.md2");
	if ( soldier ) {
		vector_t v;
		v[X] = 50;
		v[Y] = 0;
		v[Z] = 0;
		md2_skin(soldier, 0);
		md2_animate(soldier, 146, 214);
		md2_spawn(soldier, v);
	}

	md5_load("data/d3/demo/models/md5/chars/marine.md5mesh",
		"data/d3/demo/models/md5/chars/marscity/marscity_marine2_idle2.md5anim");
	return 1;
}

void clcmd_map(int s, char *arg)
{
	q2bsp_t tmp_map;

	char buf[strlen(arg) + strlen("maps/.bsp") + 1];

	snprintf(buf, sizeof(buf), "maps/%s.bsp", arg);

	tmp_map = q2bsp_load(buf);
	if ( NULL == tmp_map ) {
		con_printf("%s: map not found\n", buf);
		return;
	}

	q2bsp_free(map);
	map = tmp_map;
}

void cl_render(void)
{
	if ( map )
		q2bsp_render(map);
	md5_render();
	if ( soldier )
		md2_render(soldier);
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
