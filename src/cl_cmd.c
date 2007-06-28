/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/

#include <blackbloc.h>
#include <client.h>
#include <hud.h>

void cl_cmd_run(char *cmd)
{
	struct cl_cmd *c = cl_cmd_by_name(cmd);

	if ( !c ) {
		con_printf("%s: Unknown command\n", cmd);
		return;
	}

	c->fn(1);
}

int cl_cmd_bind(const char *k, const char *c)
{
	struct cl_cmd *cmd;
	int i;
	int f=0;

	i = sdl_keyb_code(k);
	if ( i<0 ) {
		con_printf("Unkown key: \"%s\"\n", k);
		return 0;
	}

	if ( *c == '+' ) {
		c++;
		f = 1;
	}

	cmd = cl_cmd_by_name(c);
	if ( cmd == NULL ) {
		con_printf("Unkown command: \"%s\"\n", c);
		return 0;
	}

	sdl_keyb_bind(i, f, cmd);
	return 1;
}

static void clcmd_console(int s)
{
	hud_toggle_console();
}

static void clcmd_quit(int s)
{
	cl_alive = 0;
}

/* List of all registered client commands */
static struct cl_cmd command[]={
	{clcmd_forwards, "forwards", "Walk forwards"},
	{clcmd_backwards, "backwards", "Walk backwards"},
	{clcmd_strafe_left, "strafe_left", "Strafe left"},
	{clcmd_strafe_right, "strafe_right", "Strafe right"},
	{clcmd_jump, "jump", "Jump"},
	{clcmd_crouch, "crouch", "Crouch"},
	{clcmd_quit, "quit", "Exit the game"},
	{clcmd_console, "console", "Toggle the console"},
};

/* Helper for qsort/bsearch */
static int cmd_compar(const void *aa, const void *bb)
{
	struct cl_cmd *a=(struct cl_cmd *)aa;
	struct cl_cmd *b=(struct cl_cmd *)bb;

	return strcasecmp(a->name, b->name);
}

/* Search for a command by name */
struct cl_cmd *cl_cmd_by_name(const char *cmd)
{
	struct cl_cmd x={.name=cmd};
	return bsearch(&x, command,
		sizeof(command)/sizeof(*command),
		sizeof(*command), cmd_compar);
}

/* Qsort the array for bsearch */
static void __attribute__((constructor))
cl_cmd_init(void) {
	qsort(command, sizeof(command)/sizeof(*command),
		sizeof(*command), cmd_compar);
}
