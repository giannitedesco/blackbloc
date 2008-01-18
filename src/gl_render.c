/*
* This file is part of blackbloc
* Copyright (c) 2002 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/
#include <unistd.h>
#include <SDL.h>

#include <blackbloc.h>
#include <client.h>
#include <hud.h>
#include <gl_render.h>

static unsigned int vid_x;
static unsigned int vid_y;
static unsigned int vid_depth;
static unsigned int vid_fullscreen;
static unsigned int vid_wireframe;
static SDL_Surface *screen;

unsigned int gl_render_vidx(void)
{
	return vid_x;
}

unsigned int gl_render_vidy(void)
{
	return vid_y;
}

unsigned int gl_render_toggle_wireframe(void)
{
	return (vid_wireframe = !!vid_wireframe);
}

/* Initialise openGL */
int gl_init(unsigned int vx, unsigned int vy, unsigned int d, unsigned int fs)
{
	int flags = SDL_OPENGL;

	if ( screen ) {
		con_printf("TODO: Fix mode change\n");
		return 0;
	}

	if ( fs )
		flags |= SDL_FULLSCREEN;

	/* Need 5 bits of color depth for each color */
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

	/* Enable double buffering */
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	/* Setup the depth buffer, 16 deep */
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

	/* Setup the SDL display */
	screen = SDL_SetVideoMode(vx,vy,d,flags);
	if ( screen == NULL ) {
		con_printf("gl_init: SDL_SetVideoMode: %s\n", SDL_GetError());
		return 0;
	}

	vid_x = vx;
	vid_y = vy;
	vid_depth = d;
	vid_fullscreen = fs;

	SDL_WM_SetCaption("blackbloc", "blackbloc");
	SDL_ShowCursor(0);

	/* Print out OpenGL info */
	con_printf("gl_vidmode: %i x %i x %i\n", vid_x, vid_y, vid_depth);
	con_printf("gl_vendor: %s\n", glGetString(GL_VENDOR));
	con_printf("gl_renderer: %s\n", glGetString(GL_RENDERER));
	con_printf("gl_version: %s\n", glGetString(GL_VERSION));
	con_printf("extensions: %s\n", glGetString(GL_EXTENSIONS));

	glClearColor(0, 0, 0, 1);
	glShadeModel(GL_SMOOTH);
	glDisable(GL_LIGHTING);

	return 1;
}

/* Help us to setup the viewing frustum */
static void gl_frustum(GLdouble fovy,
		GLdouble aspect,
		GLdouble zNear,
		GLdouble zFar)
{
	GLdouble xmin, xmax, ymin, ymax;

	ymax = zNear * tan(fovy * M_PI / 360.0);
	ymin = -ymax;

	xmin = ymin * aspect;
	xmax = ymax * aspect;

	glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}

/* Prepare OpenGL for 3d rendering */
static void gl_3d(void)
{
	/* Reset current viewport */
	glViewport(0, 0, vid_x, vid_y);

	/* Use a 45 degree each side viewing frustum */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gl_frustum(90.0f, (GLdouble)vid_x / (GLdouble)vid_y, 4, 4069);

	/* Reset the modelview matrix */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	/* Finish off setting up the depth buffer */
	glClearDepth(1.0f);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	/* Enable alpha blending */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);

	/* Use back-face culling */
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	/* Enable textures */
	if ( vid_wireframe ) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
	}else{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_TEXTURE_2D);
	}
}

/* Prepare OpenGL for 2d rendering */
static void gl_2d(void)
{
	/* Use an orthogonal projection */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, vid_x, vid_y, 0, -99999, 99999);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	/* So 2d always overlays 3d */
	glDisable(GL_DEPTH_TEST);

	/* So we can wind in any direction */
	glDisable(GL_CULL_FACE);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

/* Global blends are done here */
static void blend_render(void)
{
	glDisable(GL_TEXTURE_2D);
	glColor4f(me.blend[R], me.blend[G],
		me.blend[B], me.blend[A]);
	glBegin(GL_QUADS);
	glVertex2f(0, 0);
	glVertex2f(vid_x, 0);
	glVertex2f(vid_x, vid_y);
	glVertex2f(0, vid_y);
	glEnd();
	glEnable(GL_TEXTURE_2D);
	glColor4f(1.0, 1.0, 1.0, 1.0);
}

/* Main rendering loop */
void gl_render()
{
	vector_t tmp;

	gl_3d();
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	cl_viewangles(tmp);
	v_invert(tmp);
	glRotatef(tmp[PITCH], 1, 0, 0);
	glRotatef(tmp[YAW], 0, 1, 0);
	glRotatef(tmp[ROLL], 0, 0, 1);

	cl_origin(tmp);
	v_invert(tmp);
	glTranslatef(tmp[X], tmp[Y], tmp[Z]);

	cl_render();

	/* Render 2d stuff (hud/console etc.) */
	gl_2d();
	blend_render();
	hud_render();

	/* w00t */
	SDL_GL_SwapBuffers();
}
