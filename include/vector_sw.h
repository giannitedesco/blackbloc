/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*
* Software reference implementation of vector operations.
*/
#ifndef __VECTOR_SW_HEADER_INCLUDED__
#define __VECTOR_SW_HEADER_INCLUDED__

/* func: square root
 * desc: gets the square root of a scalar value
 * notes: does not have to be amazingly accurate
*/
#ifndef V_SQRT
#define v_sqrt(s) sqrt(s)
#endif

/* func: zero
 * desc: resets a vector to zeros (scale 1)
 * notes: v is clobberd (duh)
*/
#ifndef V_ZERO
static inline void __vfunc
v_zero(vector_t v)
{
	v[X]=0;
	v[Y]=0;
	v[Z]=0;
}
#endif

/* func: copy
 * desc: copies s in to d
 * notes: d is clobbered (duh)
*/
#ifndef V_COPY
static inline void __vfunc
v_copy(vector_t d, const vector_t s)
{
	d[X]=s[X];
	d[Y]=s[Y];
	d[Z]=s[Z];
}
#endif

/* func: invert
 * desc: inverts a vector
 * notes: v is modified in place
*/
#ifndef V_INVERT
static inline void __vfunc
v_invert(vector_t v)
{
	v[X] = -v[X];
	v[Y] = -v[Y];
	v[Z] = -v[Z];
}
#endif

/* func: scale
 * desc: scales a vector by a scaling factor
 * notes: v is modified in place
*/
#ifndef V_SCALE
static inline void __vfunc
v_scale(vector_t v, const scalar_t s)
{
	v[X] *= s;
	v[Y] *= s;
	v[Z] *= s;
}
#endif

/* func: length
 * desc: gets the length of a vector
 * notes: none
*/
#ifndef V_LEN
static inline scalar_t
v_len(const vector_t v)
{
	scalar_t len;

	len = (v[X] * v[X]) +
		(v[Y] * v[Y]) +
		(v[Z] * v[Z]);

	return v_sqrt(len);
}
#endif

/* func: normalize
 * desc: normalizes a vector in place
 * notes: v is modified
*/
#ifndef V_NORMALIZE
static inline void __vfunc
v_normalize(vector_t v)
{
	scalar_t len=v_len(v);

	if ( len==0.0f )
		return;

	len = 1/len;
	v[X] *= len;
	v[Y] *= len;
	v[Z] *= len;
}
#endif

/* func: addition
 * desc: adds v1 to v2 and stores in d
 * notes: d is modified
*/
#ifndef V_ADD
static inline void __vfunc
v_add(vector_t d, const vector_t v1, const vector_t v2)
{
	d[X]=v1[X]+v2[X];
	d[Y]=v1[Y]+v2[Y];
	d[Z]=v1[Z]+v2[Z];
}
#endif

/* func: subtraction
 * desc: subtracts v1 from v2 and stores in d
 * notes: d is modified
*/
#ifndef V_SUB
static inline void __vfunc
v_sub(vector_t d, const vector_t v1, const vector_t v2)
{
	d[X]=v1[X]-v2[X];
	d[Y]=v1[Y]-v2[Y];
	d[Z]=v1[Z]-v2[Z];
}
#endif

/* func: Cross product
 * desc: creates a vector normal to 2 vectors in an L shape
 * notes: none
*/
#ifndef V_CROSSPRODUCT
static inline void __vfunc
v_crossproduct(vector_t d, const vector_t v1, const vector_t v2)
{
	d[X] = (v1[Y] * v2[Z]) - (v1[Z] * v2[Y]);
	d[Y] = (v1[Z] * v2[X]) - (v1[X] * v2[Z]);
	d[Z] = (v1[X] * v2[Y]) - (v1[Y] * v2[X]);
}
#endif

/* func: Dot product
 * desc: Gets the difference between 2 vectors
 * notes: Vectors must be normalized
*/
#ifndef V_DOTPRODUCT
static inline scalar_t __vfunc
v_dotproduct(const vector_t v1, const vector_t v2)
{
	return (v1[X] * v2[X]) +
		(v1[Y] * v2[Y]) +
		(v1[Z] * v2[Z]);
}
#endif

/* func: Multiply and add
 * desc: d = v1 + (scale*v2)
 * notes: d is modified
*/
#ifndef V_MULTADD
static inline void __vfunc
v_multadd(vector_t d, scalar_t scale, const vector_t v1, const vector_t v2)
{
	d[X] = v1[X] + scale * v2[X];
	d[Y] = v1[Y] + scale * v2[Y];
	d[Z] = v1[Z] + scale * v2[Z];
}
#endif

#endif /* __VECTOR_SW_HEADER_INCLUDED__ */
