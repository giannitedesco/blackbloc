/***
*
*	Copyright (c) 1998, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
****/
// studio_render.cpp: routines for drawing Half-Life 3DStudio models
// updates:
// 1-4-99	fixed AdvanceFrame wraping bug

#include <string.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <bytesex.h>
#include <studio_model.h>
#include <render_settings.h>

vec3_t g_vright = {50, 50, 0}; /* to viewers right, for chrome */

////////////////////////////////////////////////////////////////////////

static vec3_t	g_xformverts[MAXSTUDIOVERTS];	// transformed vertices
static vec3_t	g_lightvalues[MAXSTUDIOVERTS];	// light surface normals
static vec3_t	*g_pxformverts;
static vec3_t	*g_pvlightvalues;

static vec3_t	g_lightvec;			// light vector in model reference frame
static vec3_t	g_blightvec[MAXSTUDIOBONES];	// light vectors in bone reference frames
static int	g_ambientlight;			// ambient world light
static float	g_shadelight;			// direct world light
vec3_t	g_lightcolor;

static int	g_smodels_total;		// cookie

static float	g_bonetransform[MAXSTUDIOBONES][3][4];	// bone transformation matrix

static int	g_chrome[MAXSTUDIOVERTS][2];	// texture coords for surface normals
static int	g_chromeage[MAXSTUDIOBONES];	// last time chrome vectors were updated
static vec3_t	g_chromeup[MAXSTUDIOBONES];	// chrome vector "up" in bone reference frames
static vec3_t	g_chromeright[MAXSTUDIOBONES];	// chrome vector "right" in bone reference frames

////////////////////////////////////////////////////////////////////////

static void sm_CalcBoneAdj(struct studio_model *sm)
{
	unsigned int					i, j;
	float				value;
	struct studio_bonecontroller *p;
	
	p = (void *)((uint8_t *)sm->m_pstudiohdr +
			sm->m_pstudiohdr->bonecontrollerindex);

	for (j = 0; j < sm->m_pstudiohdr->numbonecontrollers; j++)
	{
		i = p[j].index;
		if (i <= 3)
		{
			// check for 360% wrapping
			if (p[j].type & STUDIO_RLOOP)
			{
				value = sm->m_controller[i] * (360.0/256.0) + p[j].start;
			}else{
				value = sm->m_controller[i] / 255.0;
				if (value < 0) value = 0;
				if (value > 1.0) value = 1.0;
				value = (1.0 - value) * p[j].start + value * p[j].end;
			}
			// Con_DPrintf( "%d %d %f : %f\n", sm->m_controller[j], sm->m_prevcontroller[j], value, dadt );
		}
		else
		{
			value = sm->m_mouth / 64.0;
			if (value > 1.0) value = 1.0;
			value = (1.0 - value) * p[j].start + value * p[j].end;
			// Con_DPrintf("%d %f\n", mouthopen, value );
		}
		switch(p[j].type & STUDIO_TYPES)
		{
		case STUDIO_XR:
		case STUDIO_YR:
		case STUDIO_ZR:
			sm->m_adj[j] = value * (Q_PI / 180.0);
			break;
		case STUDIO_X:
		case STUDIO_Y:
		case STUDIO_Z:
			sm->m_adj[j] = value;
			break;
		}
	}
}


static void sm_CalcBoneQuaternion(struct studio_model *sm, int frame, float s, struct studio_bone *pbone, struct studio_anim *panim, float *q )
{
	int					j, k;
	vec4_t				q1, q2;
	vec3_t				angle1, angle2;
	struct studio_animvalue	*p;

	for (j = 0; j < 3; j++)
	{
		if (panim->offset[j+3] == 0)
		{
			angle2[j] = angle1[j] = pbone->value[j+3]; // default;
		}
		else
		{
			p = (struct studio_animvalue *)((uint8_t *)panim + panim->offset[j+3]);
			k = frame;
			while (p->num.total <= k)
			{
				//printf("j=%u k=%i %u %u\n", j, k,
				//	p->num.total, p->num.valid);
				k -= p->num.total;
				p += p->num.valid + 1;
			}
			// Bah, missing blend!
			if (p->num.valid > k)
			{
				angle1[j] = (int16_t)le_16(p[k+1].value);
				//printf("j=%u angle1=%f\n", j, angle1[j]);

				if (p->num.valid > k + 1)
				{
					angle2[j] = (int16_t)le_16(p[k+2].value);
				}
				else
				{
					if (p->num.total > k + 1)
						angle2[j] = angle1[j];
					else
						angle2[j] = (int16_t)le_16(p[p->num.valid+2].value);
				}
			}
			else
			{
				angle1[j] = (int16_t)le_16(p[p->num.valid].value);
				if (p->num.total > k + 1)
				{
					angle2[j] = angle1[j];
				}
				else
				{
					angle2[j] = (int16_t)le_16(p[p->num.valid + 2].value);
				}
			}
			angle1[j] = pbone->value[j+3] + angle1[j] * pbone->scale[j+3];
			angle2[j] = pbone->value[j+3] + angle2[j] * pbone->scale[j+3];
		}

		if (pbone->bonecontroller[j+3] != -1)
		{
			angle1[j] += sm->m_adj[pbone->bonecontroller[j+3]];
			angle2[j] += sm->m_adj[pbone->bonecontroller[j+3]];
		}
	}

	if (!VectorCompare( angle1, angle2 ))
	{
		AngleQuaternion( angle1, q1 );
		AngleQuaternion( angle2, q2 );
		QuaternionSlerp( q1, q2, s, q );
		//if ( pbone->parent == -1 )
			//printf(" -- %f/%f = %f\n", angle1[2], angle2[2], q[2]);
	}
	else
	{
		AngleQuaternion( angle1, q );
	}
}


static void sm_CalcBonePosition(struct studio_model *sm,  int frame, float s, struct studio_bone *pbone, struct studio_anim *panim, float *pos )
{
	int					j, k;
	struct studio_animvalue	*p;

	for (j = 0; j < 3; j++)
	{
		pos[j] = pbone->value[j]; // default;
		if (panim->offset[j] != 0)
		{
			p = (struct studio_animvalue *)((uint8_t *)panim + panim->offset[j]);
			
			k = frame;
			// find span of values that includes the frame we want
			while (p->num.total <= k)
			{
				k -= p->num.total;
				p += p->num.valid + 1;
			}
			// if we're inside the span
			if (p->num.valid > k)
			{
				// and there's more data in the span
				if (p->num.valid > k + 1)
				{
					pos[j] += ((int16_t)le_16(p[k+1].value) * (1.0 - s) + s * (int16_t)le_16(p[k+2].value)) * pbone->scale[j];
				}
				else
				{
					pos[j] += (int16_t)le_16(p[k+1].value) * pbone->scale[j];
				}
			}
			else
			{
				// are we at the end of the repeating values section and there's another section with data?
				if (p->num.total <= k + 1)
				{
					pos[j] += ((int16_t)le_16(p[p->num.valid].value) * (1.0 - s) + s * (int16_t)le_16(p[p->num.valid + 2].value)) * pbone->scale[j];
				}
				else
				{
					pos[j] += (int16_t)le_16(p[p->num.valid].value) * pbone->scale[j];
				}
			}
		}
		if (pbone->bonecontroller[j] != -1)
		{
			pos[j] += sm->m_adj[pbone->bonecontroller[j]];
		}
	}
}


static void sm_CalcRotations (struct studio_model *sm,  vec3_t *pos, vec4_t *q, struct studio_seqdesc *pseqdesc, struct studio_anim *panim, float f )
{
	unsigned int i;
	unsigned int frame;
	struct studio_bone		*pbone;
	float				s;

	frame = lrintf(fabsf(f));
	s = lrintf(fabsf(nearbyint(f) - f));

	// add in programatic controllers
	sm_CalcBoneAdj(sm);

	pbone		= (struct studio_bone *)((uint8_t *)sm->m_pstudiohdr + sm->m_pstudiohdr->boneindex);
	for (i = 0; i < sm->m_pstudiohdr->numbones; i++, pbone++, panim++) 
	{
		sm_CalcBoneQuaternion(sm, frame, s, pbone, panim, q[i] );
		sm_CalcBonePosition(sm, frame, s, pbone, panim, pos[i] );
	}

	if (pseqdesc->motiontype & STUDIO_X)
		pos[pseqdesc->motionbone][0] = 0.0;
	if (pseqdesc->motiontype & STUDIO_Y)
		pos[pseqdesc->motionbone][1] = 0.0;
	if (pseqdesc->motiontype & STUDIO_Z)
		pos[pseqdesc->motionbone][2] = 0.0;
}


static struct studio_anim * sm_GetAnim(struct studio_model *sm,  struct studio_seqdesc *pseqdesc )
{
	struct studio_seqgroup	*pseqgroup;
	pseqgroup = (struct studio_seqgroup *)((uint8_t *)sm->m_pstudiohdr + sm->m_pstudiohdr->seqgroupindex) + pseqdesc->seqgroup;

	if (pseqdesc->seqgroup == 0)
	{
		return (struct studio_anim *)((uint8_t *)sm->m_pstudiohdr + pseqgroup->data + pseqdesc->animindex);
	}

	return (struct studio_anim *)((uint8_t *)sm->m_panimhdr[pseqdesc->seqgroup] + pseqdesc->animindex);
}


static void sm_SlerpBones(struct studio_model *sm,  vec4_t q1[], vec3_t pos1[], vec4_t q2[], vec3_t pos2[], float s )
{
	unsigned int i;
	vec4_t		q3;
	float		s1;

	if (s < 0) s = 0;
	else if (s > 1.0) s = 1.0;

	s1 = 1.0 - s;

	for (i = 0; i < sm->m_pstudiohdr->numbones; i++)
	{
		QuaternionSlerp(q1[i], q2[i], s, q3 );
		q1[i][0] = q3[0];
		q1[i][1] = q3[1];
		q1[i][2] = q3[2];
		q1[i][3] = q3[3];
		pos1[i][0] = pos1[i][0] * s1 + pos2[i][0] * s;
		pos1[i][1] = pos1[i][1] * s1 + pos2[i][1] * s;
		pos1[i][2] = pos1[i][2] * s1 + pos2[i][2] * s;
	}
}


void sm_AdvanceFrame(struct studio_model *sm,  float dt)
{
	struct studio_seqdesc	*p;

	if (!sm->m_pstudiohdr)
		return;

	p = (void *)((uint8_t *)sm->m_pstudiohdr + sm->m_pstudiohdr->seqindex)
		+ sm->m_sequence;

	if (dt > 0.1)
		dt = 0.1f;
	sm->m_frame += dt * p->fps;

	if (p->numframes <= 1) {
		sm->m_frame = 0.0;
	}else{
		// wrap
		sm->m_frame -= (int)(sm->m_frame / (p->numframes - 1)) * (p->numframes - 1);
	}
}


int sm_SetFrame(struct studio_model *sm,  int nFrame )
{
	if (nFrame == -1)
		return sm->m_frame;

	if (!sm->m_pstudiohdr)
		return 0;

	struct studio_seqdesc	*pseqdesc;
	pseqdesc = (struct studio_seqdesc *)((uint8_t *)sm->m_pstudiohdr + sm->m_pstudiohdr->seqindex) + sm->m_sequence;

	sm->m_frame = nFrame;

	if (pseqdesc->numframes <= 1) {
		sm->m_frame = 0.0;
	}else{
		// wrap
		sm->m_frame = (float)(nFrame % pseqdesc->numframes);
	}

	return sm->m_frame;
}


static void sm_SetUpBones (struct studio_model *sm)
{
	unsigned int i;

	struct studio_bone		*pbones;
	struct studio_seqdesc	*pseqdesc;
	struct studio_anim		*panim;

	static vec3_t		pos[MAXSTUDIOBONES];
	float				bonematrix[3][4];
	static vec4_t		q[MAXSTUDIOBONES];

	static vec3_t		pos2[MAXSTUDIOBONES];
	static vec4_t		q2[MAXSTUDIOBONES];
	static vec3_t		pos3[MAXSTUDIOBONES];
	static vec4_t		q3[MAXSTUDIOBONES];
	static vec3_t		pos4[MAXSTUDIOBONES];
	static vec4_t		q4[MAXSTUDIOBONES];


	if (sm->m_sequence >=  sm->m_pstudiohdr->numseq) {
		sm->m_sequence = 0;
	}

	pseqdesc = (struct studio_seqdesc *)((uint8_t *)sm->m_pstudiohdr + sm->m_pstudiohdr->seqindex) + sm->m_sequence;

	panim = sm_GetAnim(sm, pseqdesc );
	sm_CalcRotations(sm, pos, q, pseqdesc, panim, sm->m_frame);

	if (pseqdesc->numblends > 1)
	{
		float				s;

		panim += sm->m_pstudiohdr->numbones;
		sm_CalcRotations(sm, pos2, q2, pseqdesc, panim, sm->m_frame);
		s = sm->m_blending[0] / 255.0;

		sm_SlerpBones(sm, q, pos, q2, pos2, s );

		if (pseqdesc->numblends == 4)
		{
			panim += sm->m_pstudiohdr->numbones;
			sm_CalcRotations(sm, pos3, q3, pseqdesc, panim, sm->m_frame);

			panim += sm->m_pstudiohdr->numbones;
			sm_CalcRotations(sm, pos4, q4, pseqdesc, panim, sm->m_frame);

			s = sm->m_blending[0] / 255.0;
			sm_SlerpBones(sm, q3, pos3, q4, pos4, s );

			s = sm->m_blending[1] / 255.0;
			sm_SlerpBones(sm, q, pos, q3, pos3, s );
		}
	}

	pbones = (struct studio_bone *)((uint8_t *)sm->m_pstudiohdr + sm->m_pstudiohdr->boneindex);

	for (i = 0; i < sm->m_pstudiohdr->numbones; i++) {
		QuaternionMatrix( q[i], bonematrix );

		bonematrix[0][3] = pos[i][0];
		bonematrix[1][3] = pos[i][1];
		bonematrix[2][3] = pos[i][2];

		if (pbones[i].parent == -1) {
			//if ( bonematrix[0][3] < 0.0005 )
			//	bonematrix[0][3] = 2783.318115;
			//printf("%f: %f\n", sm->m_frame, q[i][2]);
			memcpy(g_bonetransform[i], bonematrix, sizeof(float) * 12);
		} 
		else {
			R_ConcatTransforms(g_bonetransform[pbones[i].parent], bonematrix, g_bonetransform[i]);
		}
	}
}



/*
================
sm_TransformFinalVert
================
*/
static void sm_Lighting(float *lv, int bone, int flags, vec3_t normal)
{
	float 	illum;
	float	lightcos;

	illum = g_ambientlight;

	if (flags & STUDIO_NF_FLATSHADE)
	{
		illum += g_shadelight * 0.8;
	} 
	else 
	{
		float r;
		lightcos = DotProduct (normal, g_blightvec[bone]); // -1 colinear, 1 opposite

		if (lightcos > 1.0)
			lightcos = 1;

		illum += g_shadelight;

		r = 1.5f /* g_lambert */;
		if (r <= 1.0) r = 1.0;

		lightcos = (lightcos + (r - 1.0)) / r; 		// do modified hemispherical lighting
		if (lightcos > 0.0) 
		{
			illum -= g_shadelight * lightcos; 
		}
		if (illum <= 0)
			illum = 0;
	}

	if (illum > 255) 
		illum = 255;
	*lv = illum / 255.0;	// Light from 0 to 1.0
}


void sm_Chrome (struct studio_model *sm, int *pchrome, int bone, vec3_t normal)
{
	float n;

	if (g_chromeage[bone] != g_smodels_total)
	{
		// calculate vectors from the viewer to the bone. This roughly adjusts for position
		vec3_t chromeupvec;		// g_chrome t vector in world reference frame
		vec3_t chromerightvec;	// g_chrome s vector in world reference frame
		vec3_t tmp;				// vector pointing at bone in world reference frame
		VectorScale( sm->m_origin, -1, tmp );
		tmp[0] += g_bonetransform[bone][0][3];
		tmp[1] += g_bonetransform[bone][1][3];
		tmp[2] += g_bonetransform[bone][2][3];
		VectorNormalize( tmp );
		CrossProduct( tmp, g_vright, chromeupvec );
		VectorNormalize( chromeupvec );
		CrossProduct( tmp, chromeupvec, chromerightvec );
		VectorNormalize( chromerightvec );

		VectorIRotate( chromeupvec, g_bonetransform[bone], g_chromeup[bone] );
		VectorIRotate( chromerightvec, g_bonetransform[bone], g_chromeright[bone] );

		g_chromeage[bone] = g_smodels_total;
	}

	// calc s coord
	n = DotProduct( normal, g_chromeright[bone] );
	pchrome[0] = (n + 1.0) * 32; // FIX: make this a float

	// calc t coord
	n = DotProduct( normal, g_chromeup[bone] );
	pchrome[1] = (n + 1.0) * 32; // FIX: make this a float
}


/*
================
sm_SetupLighting
	set some global variables based on entity position
inputs:
outputs:
	g_ambientlight
	g_shadelight
================
*/
static void sm_SetupLighting (struct studio_model *sm)
{
	unsigned int i;
	g_ambientlight = 32;
	g_shadelight = 192;

	g_lightvec[0] = 0;
	g_lightvec[1] = 0;
	g_lightvec[2] = -1.0;

	/* FIXME */
	g_lightcolor[0] = 0.0;//g_viewerSettings.lColor[0];
	g_lightcolor[1] = 0.0;//g_viewerSettings.lColor[1];
	g_lightcolor[2] = 0.0;//g_viewerSettings.lColor[2];

	// TODO: only do it for bones that actually have textures
	for (i = 0; i < sm->m_pstudiohdr->numbones; i++)
	{
		VectorIRotate( g_lightvec, g_bonetransform[i], g_blightvec[i] );
	}
}


/*
=================
sm_SetupModel
	based on the body part, figure out which mesh it should be using.
inputs:
	currententity
outputs:
	pstudiomesh
	pmdl
=================
*/

static void sm_SetupModel(struct studio_model *sm, unsigned int bodypart)
{
	unsigned int index;

	if (bodypart > sm->m_pstudiohdr->numbodyparts)
	{
		// Con_DPrintf ("sm_SetupModel: no such bodypart %d\n", bodypart);
		bodypart = 0;
	}

	struct studio_bodypart   *pbodypart = (struct studio_bodypart *)((uint8_t *)sm->m_pstudiohdr + sm->m_pstudiohdr->bodypartindex) + bodypart;

	index = sm->m_bodynum / pbodypart->base;
	index = index % pbodypart->nummodels;

	sm->m_pmodel = (struct studio_dmodel *)((uint8_t *)sm->m_pstudiohdr + pbodypart->modelindex) + index;
}


void drawBox(vec3_t *v)
{
	int i;

	glBegin (GL_QUAD_STRIP);
	for (i = 0; i < 10; i++)
		glVertex3fv (v[i & 7]);
	glEnd ();
	
	glBegin  (GL_QUAD_STRIP);
	glVertex3fv (v[6]);
	glVertex3fv (v[0]);
	glVertex3fv (v[4]);
	glVertex3fv (v[2]);
	glEnd ();

	glBegin  (GL_QUAD_STRIP);
	glVertex3fv (v[1]);
	glVertex3fv (v[7]);
	glVertex3fv (v[3]);
	glVertex3fv (v[5]);
	glEnd ();

}

void sm_scaleMeshes(struct studio_model *sm, float scale)
{
	unsigned int i, j, k;

	if (!sm->m_pstudiohdr)
		return;

	// scale verts
	int tmp = sm->m_bodynum;
	for (i = 0; i < sm->m_pstudiohdr->numbodyparts; i++)
	{
		struct studio_bodypart *pbodypart = (struct studio_bodypart *)((uint8_t *)sm->m_pstudiohdr + sm->m_pstudiohdr->bodypartindex) + i;
		for (j = 0; j < pbodypart->nummodels; j++)
		{
			sm_SetBodygroup(sm, i, j);
			sm_SetupModel(sm, i);

			vec3_t *pstudioverts = (vec3_t *)((uint8_t *)sm->m_pstudiohdr + sm->m_pmodel->vertindex);

			for (k = 0; k < sm->m_pmodel->numverts; k++)
				VectorScale (pstudioverts[k], scale, pstudioverts[k]);
		}
	}

	sm->m_bodynum = tmp;

	// scale complex hitboxes
	struct studio_bbox *pbboxes = (struct studio_bbox *) ((uint8_t *) sm->m_pstudiohdr + sm->m_pstudiohdr->hitboxindex);
	for (i = 0; i < sm->m_pstudiohdr->numhitboxes; i++)
	{
		VectorScale (pbboxes[i].bbmin, scale, pbboxes[i].bbmin);
		VectorScale (pbboxes[i].bbmax, scale, pbboxes[i].bbmax);
	}

	// scale bounding boxes
	struct studio_seqdesc *pseqdesc = (struct studio_seqdesc *)((uint8_t *)sm->m_pstudiohdr + sm->m_pstudiohdr->seqindex);
	for (i = 0; i < sm->m_pstudiohdr->numseq; i++)
	{
		VectorScale (pseqdesc[i].bbmin, scale, pseqdesc[i].bbmin);
		VectorScale (pseqdesc[i].bbmax, scale, pseqdesc[i].bbmax);
	}

	// maybe scale exeposition, pivots, attachments
}



/*
================
sm_DrawModel
inputs:
	currententity
	r_entorigin
================
*/
static void sm_DrawPoints (struct studio_model *sm);
void sm_DrawModel(struct studio_model *sm)
{
	unsigned int i;

	if (!sm->m_pstudiohdr)
		return;

	g_smodels_total++; // render data cache cookie

	g_pxformverts = &g_xformverts[0];
	g_pvlightvalues = &g_lightvalues[0];

	if (sm->m_pstudiohdr->numbodyparts == 0)
		return;

//	glPushMatrix ();

//    glTranslatef (sm->m_origin[0],  sm->m_origin[1],  sm->m_origin[2]);

//    glRotatef (sm->m_angles[1],  0, 0, 1);
//   glRotatef (sm->m_angles[0],  0, 1, 0);
 //  glRotatef (sm->m_angles[2],  1, 0, 0);

	sm_SetUpBones(sm);
	sm_SetupLighting(sm);

	for (i=0 ; i < sm->m_pstudiohdr->numbodyparts ; i++) 
	{
		sm_SetupModel(sm, i);
		if (g_viewerSettings.transparency > 0.0f)
			sm_DrawPoints(sm);
	}

	// draw bones
	if (g_viewerSettings.showBones && !g_viewerSettings.use3dfx)
	{
		struct studio_bone *pbones = (struct studio_bone *) ((uint8_t *) sm->m_pstudiohdr + sm->m_pstudiohdr->boneindex);
		glDisable (GL_TEXTURE_2D);
		glDisable (GL_DEPTH_TEST);

		for (i = 0; i < sm->m_pstudiohdr->numbones; i++)
		{
			if (pbones[i].parent >= 0)
			{
				glPointSize (3.0f);
				glColor3f (1, 0.7f, 0);
				glBegin (GL_LINES);
				glVertex3f (g_bonetransform[pbones[i].parent][0][3], g_bonetransform[pbones[i].parent][1][3], g_bonetransform[pbones[i].parent][2][3]);
				glVertex3f (g_bonetransform[i][0][3], g_bonetransform[i][1][3], g_bonetransform[i][2][3]);
				glEnd ();

				glColor3f (0, 0, 0.8f);
				glBegin (GL_POINTS);
				if (pbones[pbones[i].parent].parent != -1)
					glVertex3f (g_bonetransform[pbones[i].parent][0][3], g_bonetransform[pbones[i].parent][1][3], g_bonetransform[pbones[i].parent][2][3]);
				glVertex3f (g_bonetransform[i][0][3], g_bonetransform[i][1][3], g_bonetransform[i][2][3]);
				glEnd ();
			}
			else
			{
				// draw parent bone node
				glPointSize (5.0f);
				glColor3f (0.8f, 0, 0);
				glBegin (GL_POINTS);
				glVertex3f (g_bonetransform[i][0][3], g_bonetransform[i][1][3], g_bonetransform[i][2][3]);
				glEnd ();
			}
		}

		glPointSize (1.0f);
	}

	if (g_viewerSettings.showAttachments && !g_viewerSettings.use3dfx)
	{
		glDisable (GL_TEXTURE_2D);
		glDisable (GL_CULL_FACE);
		glDisable (GL_DEPTH_TEST);
		for (i = 0; i < sm->m_pstudiohdr->numattachments; i++)
		{
			struct studio_attachment *pattachments = (struct studio_attachment *) ((uint8_t *) sm->m_pstudiohdr + sm->m_pstudiohdr->attachmentindex);
			vec3_t v[4];
			VectorTransform (pattachments[i].org, g_bonetransform[pattachments[i].bone], v[0]);
			VectorTransform (pattachments[i].vectors[0], g_bonetransform[pattachments[i].bone], v[1]);
			VectorTransform (pattachments[i].vectors[1], g_bonetransform[pattachments[i].bone], v[2]);
			VectorTransform (pattachments[i].vectors[2], g_bonetransform[pattachments[i].bone], v[3]);
			glBegin (GL_LINES);
			glColor3f (1, 0, 0);
			glVertex3fv (v[0]);
			glColor3f (1, 1, 1);
			glVertex3fv (v[1]);
			glColor3f (1, 0, 0);
			glVertex3fv (v[0]);
			glColor3f (1, 1, 1);
			glVertex3fv (v[2]);
			glColor3f (1, 0, 0);
			glVertex3fv (v[0]);
			glColor3f (1, 1, 1);
			glVertex3fv (v[3]);
			glEnd();

			glPointSize (5);
			glColor3f (0, 1, 0);
			glBegin (GL_POINTS);
			glVertex3fv (v[0]);
			glEnd ();
			glPointSize (1);
		}
	}

	if (g_viewerSettings.showHitBoxes)
	{
		glDisable (GL_TEXTURE_2D);
		glDisable (GL_CULL_FACE);
		if (g_viewerSettings.transparency < 1.0f && !g_viewerSettings.use3dfx)
			glDisable (GL_DEPTH_TEST);
		else
			glEnable (GL_DEPTH_TEST);

		if (g_viewerSettings.use3dfx)
			glColor4f (1, 0, 0, 0.2f);
		else
			glColor4f (1, 0, 0, 0.5f);

		glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		for (i = 0; i < sm->m_pstudiohdr->numhitboxes; i++)
		{
			struct studio_bbox *pbboxes = (struct studio_bbox *) ((uint8_t *) sm->m_pstudiohdr + sm->m_pstudiohdr->hitboxindex);
			vec3_t v[8], v2[8], bbmin, bbmax;

			VectorCopy (pbboxes[i].bbmin, bbmin);
			VectorCopy (pbboxes[i].bbmax, bbmax);

			v[0][0] = bbmin[0];
			v[0][1] = bbmax[1];
			v[0][2] = bbmin[2];

			v[1][0] = bbmin[0];
			v[1][1] = bbmin[1];
			v[1][2] = bbmin[2];

			v[2][0] = bbmax[0];
			v[2][1] = bbmax[1];
			v[2][2] = bbmin[2];

			v[3][0] = bbmax[0];
			v[3][1] = bbmin[1];
			v[3][2] = bbmin[2];

			v[4][0] = bbmax[0];
			v[4][1] = bbmax[1];
			v[4][2] = bbmax[2];

			v[5][0] = bbmax[0];
			v[5][1] = bbmin[1];
			v[5][2] = bbmax[2];

			v[6][0] = bbmin[0];
			v[6][1] = bbmax[1];
			v[6][2] = bbmax[2];

			v[7][0] = bbmin[0];
			v[7][1] = bbmin[1];
			v[7][2] = bbmax[2];

			VectorTransform (v[0], g_bonetransform[pbboxes[i].bone], v2[0]);
			VectorTransform (v[1], g_bonetransform[pbboxes[i].bone], v2[1]);
			VectorTransform (v[2], g_bonetransform[pbboxes[i].bone], v2[2]);
			VectorTransform (v[3], g_bonetransform[pbboxes[i].bone], v2[3]);
			VectorTransform (v[4], g_bonetransform[pbboxes[i].bone], v2[4]);
			VectorTransform (v[5], g_bonetransform[pbboxes[i].bone], v2[5]);
			VectorTransform (v[6], g_bonetransform[pbboxes[i].bone], v2[6]);
			VectorTransform (v[7], g_bonetransform[pbboxes[i].bone], v2[7]);
			
			drawBox (v2);
		}
	}

	glPopMatrix ();
}

/*
================

================
*/
static void sm_DrawPoints (struct studio_model *sm)
{
	unsigned int		i, j;
	int			cmd;
	struct studio_mesh	*pmesh;
	uint8_t			*pvertbone;
	uint8_t			*pnormbone;
	vec3_t			*pstudioverts;
	vec3_t			*pstudionorms;
	struct studio_texture	*ptexture;
	float 			*av;
	float			*lv;
	float			lv_tmp;
	short			*pskinref;

	pvertbone = ((uint8_t *)sm->m_pstudiohdr + sm->m_pmodel->vertinfoindex);
	pnormbone = ((uint8_t *)sm->m_pstudiohdr + sm->m_pmodel->norminfoindex);
	ptexture = (struct studio_texture *)((uint8_t *)sm->m_ptexturehdr + sm->m_ptexturehdr->textureindex);

	pmesh = (struct studio_mesh *)((uint8_t *)sm->m_pstudiohdr + sm->m_pmodel->meshindex);

	pstudioverts = (vec3_t *)((uint8_t *)sm->m_pstudiohdr + sm->m_pmodel->vertindex);
	pstudionorms = (vec3_t *)((uint8_t *)sm->m_pstudiohdr + sm->m_pmodel->normindex);

	pskinref = (short *)((uint8_t *)sm->m_ptexturehdr + sm->m_ptexturehdr->skinindex);
	if (sm->m_skinnum != 0 && sm->m_skinnum < sm->m_ptexturehdr->numskinfamilies)
		pskinref += (sm->m_skinnum * sm->m_ptexturehdr->numskinref);

	for (i = 0; i < sm->m_pmodel->numverts; i++)
	{
		//vec3_t tmp;
		//VectorScale (pstudioverts[i], 12, tmp);
		VectorTransform (pstudioverts[i], g_bonetransform[pvertbone[i]], g_pxformverts[i]);
	}

	if (g_viewerSettings.transparency < 1.0f)
	{
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

//
// clip and draw all triangles
//

	lv = (float *)g_pvlightvalues;
	for (j = 0; j < sm->m_pmodel->nummesh; j++) 
	{
		int flags;
		flags = ptexture[pskinref[pmesh[j].skinref]].flags;
		for (i = 0; i < pmesh[j].numnorms; i++, lv += 3, pstudionorms++, pnormbone++)
		{
			sm_Lighting(&lv_tmp, *pnormbone, flags, (float *)pstudionorms);

			// FIX: move this check out of the inner loop
			if (flags & STUDIO_NF_CHROME)
				sm_Chrome(sm, g_chrome[(float (*)[3])lv - g_pvlightvalues], *pnormbone, (float *)pstudionorms );

			lv[0] = lv_tmp * g_lightcolor[0];
			lv[1] = lv_tmp * g_lightcolor[1];
			lv[2] = lv_tmp * g_lightcolor[2];
		}
	}

	 glCullFace(GL_FRONT);

	for (j = 0; j < sm->m_pmodel->nummesh; j++) 
	{
		float s, t;
		short		*ptricmds;

		pmesh = (struct studio_mesh *)((uint8_t *)sm->m_pstudiohdr + sm->m_pmodel->meshindex) + j;
		ptricmds = (short *)((uint8_t *)sm->m_pstudiohdr + pmesh->triindex);

		s = 1.0/(float)ptexture[pskinref[pmesh->skinref]].width;
		t = 1.0/(float)ptexture[pskinref[pmesh->skinref]].height;

		//glBindTexture( GL_TEXTURE_2D, ptexture[pskinref[pmesh->skinref]].index );
		glBindTexture( GL_TEXTURE_2D, pskinref[pmesh->skinref] + 30);

		if (ptexture[pskinref[pmesh->skinref]].flags & STUDIO_NF_CHROME)
		{
			while ( (cmd = *(ptricmds++)) )
			{
				if (cmd < 0)
				{
					glBegin( GL_TRIANGLE_FAN );
					cmd = -cmd;
				}
				else
				{
					glBegin( GL_TRIANGLE_STRIP );
				}


				for( ; cmd > 0; cmd--, ptricmds += 4)
				{
					// FIX: put these in as integer coords, not floats
					glTexCoord2f(g_chrome[ptricmds[1]][0]*s, g_chrome[ptricmds[1]][1]*t);
					
					lv = g_pvlightvalues[ptricmds[1]];
					glColor4f( lv[0], lv[1], lv[2], g_viewerSettings.transparency);

					av = g_pxformverts[ptricmds[0]];
					glVertex3f(av[0], av[1], av[2]);
				}
				glEnd( );
			}	
		} 
		else 
		{
			while ( (cmd = *(ptricmds++)) )
			{
				if (cmd < 0)
				{
					glBegin( GL_TRIANGLE_FAN );
					cmd = -cmd;
				}
				else
				{
					glBegin( GL_TRIANGLE_STRIP );
				}


				for( ; cmd > 0; cmd--, ptricmds += 4)
				{
					// FIX: put these in as integer coords, not floats
					glTexCoord2f(ptricmds[2]*s, ptricmds[3]*t);
					
					lv = g_pvlightvalues[ptricmds[1]];
					glColor4f( lv[0], lv[1], lv[2], g_viewerSettings.transparency);

					av = g_pxformverts[ptricmds[0]];
					glVertex3f(av[0], av[1], av[2]);
				}
				glEnd( );
			}	
		}
	}
}

