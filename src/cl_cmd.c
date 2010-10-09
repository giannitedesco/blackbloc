/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#include <SDL.h>

#include <blackbloc/blackbloc.h>
#include <blackbloc/gl_render.h>
#include <blackbloc/client.h>
#include <blackbloc/hud.h>
#include <blackbloc/sdl_keyb.h>

/* Easy string tokeniser */
int easy_explode(char *str, char split,
			char **toks, int max_toks)
{
	char *tmp;
	int tok;
	int state;

	for(tmp=str,state=tok=0; *tmp && tok <= max_toks; tmp++) {
		if ( state == 0 ) {
			if ( *tmp == split && (tok < max_toks)) {
				toks[tok++] = NULL;
			}else if ( !isspace(*tmp) ) {
				state = 1;
				toks[tok++] = tmp;
			}
		}else if ( state == 1 ) {
			if ( tok < max_toks ) {
				if ( *tmp == split || isspace(*tmp) ) {
					*tmp = '\0';
					state = 0;
				}
			}else if ( *tmp == '\n' )
				*tmp = '\0';
		}
	}

	return tok;
}

void cl_cmd_run(const char *cmd)
{
	char *tok[2];
	char *buf;
	struct cl_cmd *c;
	int ret;

	buf = strdup(cmd);
	if ( NULL == buf )
		return;
	
	ret = easy_explode(buf, 0, tok, sizeof(tok)/sizeof(*tok));
	if ( ret <= 0 ) {
		free(buf);
		return;
	}
	if ( ret == 1 )
		tok[1] = NULL;

	c = cl_cmd_by_name(tok[0]);
	if ( NULL == c ) {
		con_printf("%s: Unknown command\n", tok[0]);
		free(buf);
		return;
	}

	c->fn(1, tok[1]);
	free(buf);
}

int cl_cmd_bind(const char *k, const char *c)
{
	struct cl_cmd *cmd;
	int i;

	i = sdl_keyb_code(k);
	if ( i < 0 ) {
		con_printf("Unkown key: \"%s\"\n", k);
		return 0;
	}

	cmd = cl_cmd_by_name(c);
	if ( cmd == NULL ) {
		con_printf("Unkown command: \"%s\"\n", c);
		return 0;
	}

	sdl_keyb_bind(i, cmd);
	return 1;
}

static void clcmd_console(int s, char *arg)
{
	hud_toggle_console();
}

static void clcmd_quit(int s, char *arg)
{
	cl_alive = 0;
}

static void clcmd_wireframe(int s, char *arg)
{
	gl_render_toggle_wireframe();
}

static void clcmd_binds(int s, char *arg)
{
	sdl_keyb_print();
}

static void clcmd_help(int s, char *arg);

/* List of all registered client commands */
static struct cl_cmd command[]={
	{clcmd_wireframe, "wireframe", "Toggle wireframe rendering"},
	{clcmd_forwards, "+forwards", "Walk forwards"},
	{clcmd_binds, "binds", "Show key bindings"},
	{clcmd_map, "map", "Load map"},
	{clcmd_textures, "textures", "Toggle textures"},
	{clcmd_backwards, "+backwards", "Walk backwards"},
	{clcmd_strafe_left, "+strafe_left", "Strafe left"},
	{clcmd_strafe_right, "+strafe_right", "Strafe right"},
	{clcmd_jump, "+jump", "Jump"},
	{clcmd_crouch, "+crouch", "Crouch"},
	{clcmd_quit, "quit", "Exit the game"},
	{clcmd_help, "help", "Print a list of commands"},
	{clcmd_console, "console", "Toggle the console"},
};

static void clcmd_help(int s, char *arg)
{
	unsigned int i;
	con_printf("Command List:\n");
	for(i = 0; i < sizeof(command)/sizeof(*command); i++)
		con_printf(" %s: %s\n", command[i].name, command[i].help);
}

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
