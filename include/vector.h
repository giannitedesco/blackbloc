/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*
* Basic vector manipulation
*  o v_sqrt() - square root of a scalar
*  o v_zero() - resets a vector
*  o v_copy() - copies a vector
*  o v_reverse() - reverse a vector
*  o v_scale() - scales a vector
*  o v_len() - returns length of a vector
*  o v_normalize() - normalize the vector
*  o v_add() - add one vector to another
*  o v_sub() - subtract one vector from another
*
* Vector computations
*  o v_crossproduct() - obtain a cross-product of two vectors
*  o v_dotproduct() - obtain a dot-product of two vectors
*
* OpenGL functions
*  o v_render() - render a vector
*/
#ifndef __VECTOR_HEADER_INCLUDED__
#define __VECTOR_HEADER_INCLUDED__

#include <math.h>
#include <config.h>
#include <GL/gl.h>
#ifdef HAVE_ALTIVEC_H
#include <altivec.h>
#endif

/* Defined to the maximum possible alignment to
 * keep structure sizes the same across the board
*/
#define __valign __attribute__((aligned(16)))

/* Thus far all of our vector operations are all pure functions. The
 * compiler can better optimise them if we make that explicit
*/
#define __vfunc

/* basic scalar type */
typedef float scalar_t;

typedef float vector_t[4] __valign;

/* w is a scale value but is mostly ignored, we use it to make
 * 4x4 matrices homogenous so that all affine transformations
 * are matrix multiplications. It is also here to make hardware
 * vector units work right - they usually have 128bit registers
 * so our structure needs to be at least that size or stores
 * could clobber surrounding data.
 */

#define X 0
#define Y 1
#define Z 2
#define W 3

#define PITCH 0
#define YAW 1
#define ROLL 2

/* Include optimised functions */
#ifdef HAVE_ALTIVEC_H
/* GCC is still fucked when it comes to altivec. GCC silently ignores
 * our alignment requirements for the stack EVEN when -mabi=altivec is
 * passed, this renders the altivec useless for us unless we limit
 * ourselves to static/global variables.
 *
 * gcc version 3.2.3
 */
#if 0
#include <vector_altivec.h>
#endif
#endif

/* Include in the software implementations for all 
 * non-optimised functions */
#include <vector_sw.h>

/* func: OpenGL render
 * desc: Renders a vertex
 * notes: none
*/
static inline void __vfunc
v_render(const vector_t v)
{
	glVertex3fv((GLfloat *)v);
}

void v_angles(vector_t angles, vector_t forward, vector_t right, vector_t up);

#endif /* __VECTOR_HEADER_INCLUDED__ */
