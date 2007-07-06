/***
*
*	Copyright (c) 1998, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology(c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
****/

#ifndef INCLUDED_STUDIOMODEL
#define INCLUDED_STUDIOMODEL

#include "mathlib.h"
#include "studio.h"
#include <render_settings.h>

/* I wouldn't need these messy globals if I was using C++ */
extern vec3_t g_vright;		// needs to be set to viewer's
// right in order for chrome to work
extern float g_lambert;		// modifier for pseudo-hemispherical lighting
extern struct studio_model g_studioModel;
extern ViewerSettings g_viewerSettings;

struct studio_model {
	// entity settings
	vec3_t		m_origin;
	vec3_t		m_angles;	
	int		m_sequence;	// sequence index
	float		m_frame;	// frame
	int		m_bodynum;	// bodypart selection	
	int		m_skinnum;	// skin group selection
	uint8_t		m_controller[4];// bone controllers
	uint8_t		m_blending[2];	// animation blending
	uint8_t		m_mouth;	// mouth position
	unsigned int 	m_owntexmodel;	// do we have a modelT.mdl ?

	// internal data
	studiohdr_t	*m_pstudiohdr;
	mstudiomodel_t	*m_pmodel;

	studiohdr_t	*m_ptexturehdr;
	studiohdr_t	*m_panimhdr[32];

	vec4_t		m_adj;	// FIX: non persistant, make static
};

static inline studiohdr_t *sm_getStudioHeader(struct studio_model *sm)
{
	return sm->m_pstudiohdr;
}

static inline studiohdr_t *sm_getTextureHeader(struct studio_model *sm)
{
	return sm->m_ptexturehdr;
}

static inline studiohdr_t *sm_getAnimHeader(struct studio_model *sm, int i)
{
	return sm->m_panimhdr[i];
}

void sm_FreeModel(struct studio_model *sm);
studiohdr_t *sm_LoadModel(struct studio_model *sm, char *modelname);
int sm_PostLoadModel(struct studio_model *sm, char *modelname);
int sm_SaveModel(struct studio_model *sm, char *modelname);
void sm_DrawModel(struct studio_model *sm);
void sm_AdvanceFrame(struct studio_model *sm, float dt);
int sm_SetFrame(struct studio_model *sm, int nFrame);

void sm_ExtractBbox(struct studio_model *sm, float *mins, float *maxs);

int sm_SetSequence(struct studio_model *sm, int iSequence);
int sm_GetSequence(struct studio_model *sm);

void sm_GetSequenceInfo(struct studio_model *sm,
			float *pflFrameRate,
			float *pflGroundSpeed);

void sm_UploadTexture(struct studio_model *sm,
			mstudiotexture_t *ptexture,
			uint8_t *data, uint8_t *pal,
			int name);

float sm_SetController(struct studio_model *sm, int iController, float flValue);
float sm_SetMouth(struct studio_model *sm, float flValue);
float sm_SetBlending(struct studio_model *sm, int iBlender, float flValue);
int sm_SetBodygroup(struct studio_model *sm, int iGroup, int iValue);
int sm_SetSkin(struct studio_model *sm, int iValue);

void sm_scaleMeshes(struct studio_model *sm, float scale);
void sm_scaleBones(struct studio_model *sm, float scale);

#if 1
void sm_CalcBoneAdj(struct studio_model *sm);
void sm_CalcBoneQuaternion(struct studio_model *sm,
			int frame, float s, mstudiobone_t *pbone,
			mstudioanim_t *panim, float *q);
void sm_CalcBonePosition(struct studio_model *sm,
			int frame, float s, mstudiobone_t *pbone,
			mstudioanim_t *panim, float *pos);
void sm_CalcRotations(struct studio_model *sm,
			vec3_t *pos, vec4_t *q, mstudioseqdesc_t *pseqdesc,
			mstudioanim_t *panim, float f);
mstudioanim_t *sm_GetAnim(struct studio_model *sm, mstudioseqdesc_t *pseqdesc);
void sm_SlerpBones(struct studio_model *sm,
			vec4_t q1[], vec3_t pos1[],
			vec4_t q2[], vec3_t pos2[], float s);
void sm_SetUpBones(struct studio_model *sm);
void sm_DrawPoints(struct studio_model *sm);
void sm_Lighting(struct studio_model *sm,
			float *lv, int bone, int flags, vec3_t normal);
void sm_Chrome(struct studio_model *sm, int *chrome, int bone, vec3_t normal);
void sm_SetupLighting(struct studio_model *sm);
void sm_SetupModel(struct studio_model *sm, int bodypart);
#endif


#endif // INCLUDED_STUDIOMODEL
