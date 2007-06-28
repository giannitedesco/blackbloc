/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*/

#include <SDL.h>
#include <blackbloc.h>
#include <client.h>

static int mf, mb, ml, mr, mu, md;

void cl_viewangles(vector_t angles)
{
	v_copy(angles, me.viewangles);
}

void cl_origin(vector_t origin)
{
	vector_t tmp;

	v_copy(tmp, me.origin);
	v_sub(tmp, tmp, me.oldorigin);
	v_scale(tmp, lerp);
	v_copy(origin, me.oldorigin);
	v_add(origin, origin, tmp);
	v_add(origin, me.viewoffset, origin);
}

void cl_move(void)
{
	scalar_t angle, h;
	vector_t v;

	/* For interpolation */
	v_copy(me.oldorigin, me.origin);

	/* Calculate forward/backward movement vector */
	angle = me.viewangles[Y] * M_PI*2/360;
	v[X] = sin(angle);
	v[Y] = 0;
	v[Z] = cos(angle);
	v_scale(v, 20);

	if ( mf )
		v_sub(me.origin, me.origin, v);
	if ( mb )
		v_add(me.origin, me.origin, v);

	/* Calculate strafe movement vector */
	angle = (me.viewangles[Y]+90) * M_PI*2/360;
	v[X] = sin(angle);
	v[Y] = 0;
	v[Z] = cos(angle);

	/* Can't strafe as fast as you can run! */
	v_scale(v, 17);

	if ( ml )
		v_sub(me.origin, me.origin, v);
	if ( mr )
		v_add(me.origin, me.origin, v);

	h = (float)mu * 10.0f;
	h -= (float)md * 10.0f;
	me.origin[Y] += h;
}

void clcmd_forwards(int s)
{
	mf = s;
}

void clcmd_backwards(int s)
{
	mb = s;
}

void clcmd_strafe_left(int s)
{
	ml = s;
}

void clcmd_strafe_right(int s)
{
	mr = s;
}

void clcmd_crouch(int s)
{
	md = s;
}

void clcmd_jump(int s)
{
	mu = s;
}
