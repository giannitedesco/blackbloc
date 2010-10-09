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
	void(*fn)(int state, char *args);
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
extern float fps;
extern double lerp;

/* Main API entry points */
int cl_init(void);
void cl_frame(void);
void cl_render(void);

/* Command access API */
struct cl_cmd *cl_cmd_by_name(const char *cmd);
int cl_cmd_bind(const char *k, const char *c);
void cl_cmd_run(const char *cmd);

/* Keyboard binding API */
void sdl_keyb_bind(int, struct cl_cmd *);
void sdl_keyb_unbind(int);
int sdl_keyb_code(const char *);

/* Command hooks */
void clcmd_forwards(int, char *);
void clcmd_backwards(int, char *);
void clcmd_strafe_left(int, char *);
void clcmd_strafe_right(int, char *);
void clcmd_crouch(int, char *);
void clcmd_jump(int, char *);
void clcmd_map(int, char *);

void cl_move(void);
void cl_viewangles(vector_t angles);
void cl_origin(vector_t origin);

#endif /* __CLIENT_HEADER_INCLUDED__ */
