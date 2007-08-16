/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#ifndef __CLIENT_HEADER_INCLUDED__
#define __CLIENT_HEADER_INCLUDED__

struct playerstate {
	vector_t origin;
	vector_t oldorigin;
	vector_t viewoffset;
	vector_t viewangles;
	vector_t blend;
};

struct cl_cmd {
	void(*fn)(int);
	const char *name;
	const char *help;
};

struct cl_bind {
	struct cl_cmd *cmd;
};

/* Client state, mainly rendering stuff */
extern struct playerstate me;
extern frame_t client_frame;
extern int cl_alive;
extern int vid_x;
extern int vid_y;
extern int vid_depth;
extern int vid_fullscreen;
extern unsigned int vid_wireframe;
extern float fps;
extern float lerp;

/* Main API entry points */
int cl_init(void);
void cl_frame(void);
void cl_render(void);

/* Command access API */
struct cl_cmd *cl_cmd_by_name(const char *cmd);
int cl_cmd_bind(const char *k, const char *c);
void cl_cmd_run(char *cmd);

/* Keyboard binding API */
void sdl_keyb_bind(int, struct cl_cmd *);
void sdl_keyb_unbind(int);
int sdl_keyb_code(const char *);

/* Command hooks */
void clcmd_forwards(int);
void clcmd_backwards(int);
void clcmd_strafe_left(int);
void clcmd_strafe_right(int);
void clcmd_crouch(int);
void clcmd_jump(int);

void cl_move(void);
void cl_viewangles(vector_t angles);
void cl_origin(vector_t origin);

/* model wrappers */
static inline void
cl_model_render(struct cl_ent *e)
{
	e->s.model_ops->render(e);
}

static inline void
cl_model_load(struct cl_ent *e, const char *n)
{
	e->s.model = e->s.model_ops->load(n);
}

static inline void
cl_model_skin(struct cl_ent *e, int n)
{
	e->s.skinnum = n;
	e->s.model_ops->skin_by_index(e, n);
}

static inline void
cl_model_animate(struct cl_ent *e, aframe_t f1, aframe_t f2)
{
	e->s.start_frame = f1;
	e->s.anim_frames = f2 - f1;
	e->oldframe = 0;
	e->s.frame = 0;

	/* Recalculate extents */
	e->s.model_ops->extents(e->s.model, e->s.start_frame,
				e->s.mins, e->s.maxs);

	/* don't want to interpolate between different animation
	 * sequences, otherwise we could get weird effects */
	e->last_rendered = client_frame;
}

#endif /* __CLIENT_HEADER_INCLUDED__ */
