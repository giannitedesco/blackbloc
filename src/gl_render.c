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

#if 2
int vid_x = 1024;
int vid_y = 768;
int vid_depth = 32;
int vid_fullscreen = 1;
#else
int vid_x = 640;
int vid_y = 480;
int vid_depth = 32;
int vid_fullscreen = 0;
#endif

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
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_TEXTURE_2D);
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
