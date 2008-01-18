/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*
* This is only here if the optimised versions of the vector routines
* needs some kind of initialisation done before we can start using
* them. Kinda sucks that we use GCC extensions but fuck it, GCC is
* king.
*/
#include <compiler.h>
#include <vector.h>

void v_angles(vector_t angles, vector_t forward, vector_t up, vector_t right)
{
	float		angle;
	float		sp, sy, sr, cp, cy, cr;

	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);

	if ( forward ) {
		forward[X] = cp*sy;
		forward[Y] = -sp;
		forward[Z] = cp*cy;
	}

	if ( right ) {
		right[X] = (-1*sr*sp*sy+-1*cr*cy);
		right[Y] = -1*sr*cp;
		right[Z] = (-1*sr*sp*cy+-1*cr*-sy);
	}

	if ( up ) {
		up[X] = (cr*sp*sy+-sr*cy);
		up[Y] = cr*cp;
		up[Z] = (cr*sp*cy+-sr*-sy);
	}
}


static void __attribute__((constructor)) v__ctor(void)
{
#ifdef V_CTOR
	v_ctor();
#endif
}

static void __attribute__((destructor)) v__dtor(void)
{
#ifdef V_DTOR
	v_dtor();
#endif
}
