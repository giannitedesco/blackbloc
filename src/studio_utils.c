/***
*
*	Copyright (c) 1998, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
****/
// updates:
// 1-4-99	fixed file texture load and file read bug

/* TODO:
 *  o Fix endian bugs
 *  o Fix error checking
 *  o Integrate with generic model loader
 *  o Use gfile
*/

////////////////////////////////////////////////////////////////////////
#include <blackbloc.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <studio_model.h>
#include <render_settings.h>

//extern float			g_bonetransform[MAXSTUDIOBONES][3][4];


ViewerSettings g_viewerSettings;
struct studio_model g_studioModel;
static unsigned int bFilterTextures = 1;


////////////////////////////////////////////////////////////////////////

static int g_texnum = 3;



void sm_UploadTexture(struct studio_model *sm,
			struct studio_texture *ptexture,
			uint8_t *data, uint8_t *pal, int name)
{
	// unsigned *in, int inwidth, int inheight, unsigned *out,  int outwidth, int outheight;
	int		i, j;
	int		row1[256], row2[256], col1[256], col2[256];
	uint8_t	*pix1, *pix2, *pix3, *pix4;
	uint8_t	*tex, *out;

	// convert texture to power of 2
	int outwidth;
	for (outwidth = 1; outwidth < ptexture->width; outwidth <<= 1)
		;

	//outwidth >>= 1;
	if (outwidth > 256)
		outwidth = 256;

	int outheight;
	for (outheight = 1; outheight < ptexture->height; outheight <<= 1)
		;

	//outheight >>= 1;
	if (outheight > 256)
		outheight = 256;

	tex = out = (uint8_t *)malloc(outwidth * outheight * 4);
	if (!out)
	{
		return;
	}
/*
	int k = 0;
	for (i = 0; i < ptexture->height; i++)
	{
		for (j = 0; j < ptexture->width; j++)
		{

			in[k++] = pal[data[i * ptexture->width + j] * 3 + 0];
			in[k++] = pal[data[i * ptexture->width + j] * 3 + 1];
			in[k++] = pal[data[i * ptexture->width + j] * 3 + 2];
			in[k++] = 0xff;;
		}
	}

	gluScaleImage (GL_RGBA, ptexture->width, ptexture->height, GL_UNSIGNED_BYTE, in, outwidth, outheight, GL_UNSIGNED_BYTE, out);
	free (in);
*/

	for (i = 0; i < outwidth; i++)
	{
		col1[i] = (int) ((i + 0.25) * (ptexture->width / (float)outwidth));
		col2[i] = (int) ((i + 0.75) * (ptexture->width / (float)outwidth));
	}

	for (i = 0; i < outheight; i++)
	{
		row1[i] = (int) ((i + 0.25) * (ptexture->height / (float)outheight)) * ptexture->width;
		row2[i] = (int) ((i + 0.75) * (ptexture->height / (float)outheight)) * ptexture->width;
	}

	// scale down and convert to 32bit RGB
	for (i=0 ; i<outheight ; i++)
	{
		for (j=0 ; j<outwidth ; j++, out += 4)
		{
			pix1 = &pal[data[row1[i] + col1[j]] * 3];
			pix2 = &pal[data[row1[i] + col2[j]] * 3];
			pix3 = &pal[data[row2[i] + col1[j]] * 3];
			pix4 = &pal[data[row2[i] + col2[j]] * 3];

			out[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0])>>2;
			out[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1])>>2;
			out[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2])>>2;
			out[3] = 0xFF;
		}
	}

	glBindTexture(GL_TEXTURE_2D, name); //g_texnum);		
	glTexImage2D(GL_TEXTURE_2D, 0, 3/*??*/, outwidth, outheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, bFilterTextures ? GL_LINEAR:GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, bFilterTextures ? GL_LINEAR:GL_NEAREST);

	// ptexture->width = outwidth;
	// ptexture->height = outheight;
	//ptexture->index = name; //g_texnum;

	free(tex);
}

void sm_FreeModel(struct studio_model *sm)
{
	int textures[MAXSTUDIOSKINS];
	int i;

	if (sm->m_pstudiohdr)
		free(sm->m_pstudiohdr);

	if (sm->m_ptexturehdr && sm->m_owntexmodel)
		free(sm->m_ptexturehdr);

	sm->m_pstudiohdr = sm->m_ptexturehdr = 0;
	sm->m_owntexmodel = 0;

	for (i = 0; i < 32; i++)
	{
		if (sm->m_panimhdr[i])
		{
			free(sm->m_panimhdr[i]);
			sm->m_panimhdr[i] = 0;
		}
	}

	// deleting textures
	g_texnum -= 3;
	for (i = 0; i < g_texnum; i++)
		textures[i] = i + 3;

	glDeleteTextures (g_texnum, (const GLuint *) textures);

	g_texnum = 3;
}



struct studio_hdr *sm_LoadModel(struct studio_model *sm, char *modelname)
{
	FILE *fp;
	long size;
	void *buffer;
	uint8_t	*pin;
	struct studio_hdr *phdr;
	struct studio_texture *ptexture;
	int i;

	if (!modelname)
		return 0;

	// load the model
	fp = fopen(modelname, "rb");
	if ( fp == NULL) {
		con_printf("studio: %s: %s\n",
				modelname, load_err(NULL));
		return 0;
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	buffer = malloc(size);
	if (buffer == NULL ) {
		con_printf("studio: %s: malloc(): %s\n",
				modelname, load_err(NULL));
		fclose (fp);
		return 0;
	}

	fread(buffer, size, 1, fp);
	fclose(fp);

	pin = (uint8_t *)buffer;
	phdr = (struct studio_hdr *)pin;
	ptexture = (struct studio_texture *)(pin + phdr->textureindex);

	if (strncmp ((const char *) buffer, "IDST", 4) &&
		strncmp ((const char *) buffer, "IDSQ", 4))
	{
		con_printf("studio: %s: Bad magic\n", modelname);
		free (buffer);
		return 0;
	}

	if (!strncmp ((const char *) buffer, "IDSQ", 4) && !sm->m_pstudiohdr)
	{
		con_printf("studio: %s: IDST expected\n", modelname);
		free (buffer);
		return 0;
	}

	if (phdr->textureindex > 0 && phdr->numtextures <= MAXSTUDIOSKINS)
	{
		int n = phdr->numtextures;
		for (i = 0; i < n; i++)
		{
			// strcpy(name, mod->name);
			// strcpy(name, ptexture[i].name);
			sm_UploadTexture(sm, &ptexture[i], pin + ptexture[i].index, pin + ptexture[i].width * ptexture[i].height + ptexture[i].index, g_texnum++);
		}
	}

	// UNDONE: free texture memory

	if (!sm->m_pstudiohdr) {
		sm->m_pstudiohdr = (struct studio_hdr *)buffer;
		con_printf("studio: %s loaded OK\n", sm->m_pstudiohdr->name);
	}

	return (struct studio_hdr *)buffer;
}



int sm_PostLoadModel(struct studio_model *sm, char *modelname)
{
	int n, i;

	// preload textures
	con_printf("%i textures\n", sm->m_pstudiohdr->numtextures);
	if (sm->m_pstudiohdr->numtextures == 0)
	{
		char texturename[PATH_MAX];

		strcpy(texturename, modelname);
		strcpy(&texturename[strlen(texturename) - 4], "T.mdl");

		sm->m_ptexturehdr = sm_LoadModel(sm, texturename);
		if (!sm->m_ptexturehdr)
		{
			sm_FreeModel(sm);
			return 0;
		}
		sm->m_owntexmodel = 1;
	}
	else
	{
		sm->m_ptexturehdr = sm->m_pstudiohdr;
		sm->m_owntexmodel = 0;
	}

	// preload animations
	if (sm->m_pstudiohdr->numseqgroups > 1)
	{
		for (i = 1; i < sm->m_pstudiohdr->numseqgroups; i++)
		{
			char seqgroupname[256];

			strcpy(seqgroupname, modelname);
			sprintf(&seqgroupname[strlen(seqgroupname) - 4], "%02d.mdl", i);

			sm->m_panimhdr[i] = sm_LoadModel(sm, seqgroupname);
			if (!sm->m_panimhdr[i])
			{
				sm_FreeModel(sm);
				return 0;
			}
		}
	}

	sm_SetSequence(sm, 0);
	sm_SetController(sm, 0, 0.0f);
	sm_SetController(sm, 1, 0.0f);
	sm_SetController(sm, 2, 0.0f);
	sm_SetController(sm, 3, 0.0f);
	sm_SetMouth(sm, 0.0f);

	for (n = 0; n < sm->m_pstudiohdr->numbodyparts; n++)
		sm_SetBodygroup(sm, n, 0);

	sm_SetSkin(sm, 0);
/*
	vec3_t mins, maxs;
	ExtractBbox (mins, maxs);
	if (mins[2] < 5.0f)
		sm->m_origin[2] = -mins[2];
*/
	printf("YO\n");
	return 1;
}



int sm_SaveModel(struct studio_model *sm, char *modelname)
{
	FILE *file;

	if (!modelname)
		return 0;

	if (!sm->m_pstudiohdr)
		return 0;
	
	file = fopen(modelname, "wb");
	if (!file)
		return 0;

	fwrite(sm->m_pstudiohdr, sizeof (uint8_t), sm->m_pstudiohdr->length, file);
	fclose(file);

	// write texture model
	if (sm->m_owntexmodel && sm->m_ptexturehdr)
	{
		char texturename[256];

		strcpy(texturename, modelname);
		strcpy(&texturename[strlen(texturename) - 4], "T.mdl");

		file = fopen(texturename, "wb");
		if (file)
		{
			fwrite(sm->m_ptexturehdr, sizeof (uint8_t), sm->m_ptexturehdr->length, file);
			fclose(file);
		}
	}

	// write seq groups
	if (sm->m_pstudiohdr->numseqgroups > 1)
	{
		int i;
		for (i = 1; i < sm->m_pstudiohdr->numseqgroups; i++)
		{
			char seqgroupname[256];

			strcpy(seqgroupname, modelname);
			sprintf(&seqgroupname[strlen(seqgroupname) - 4], "%02d.mdl", i);
			file = fopen(seqgroupname, "wb");
			if (file)
			{
				fwrite(sm->m_panimhdr[i], sizeof (uint8_t), sm->m_panimhdr[i]->length, file);
				fclose(file);
			}
		}
	}

	return 1;
}



////////////////////////////////////////////////////////////////////////

int sm_GetSequence(struct studio_model *sm)
{
	return sm->m_sequence;
}

int sm_SetSequence(struct studio_model *sm, int iSequence)
{
	if (iSequence > sm->m_pstudiohdr->numseq)
		return sm->m_sequence;

	sm->m_sequence = iSequence;
	sm->m_frame = 0;

	return sm->m_sequence;
}


void sm_ExtractBbox(struct studio_model *sm, float *mins, float *maxs)
{
	struct studio_seqdesc	*pseqdesc;

	pseqdesc = (struct studio_seqdesc *)((uint8_t *)sm->m_pstudiohdr + sm->m_pstudiohdr->seqindex);
	
	mins[0] = pseqdesc[ sm->m_sequence ].bbmin[0];
	mins[1] = pseqdesc[ sm->m_sequence ].bbmin[1];
	mins[2] = pseqdesc[ sm->m_sequence ].bbmin[2];

	maxs[0] = pseqdesc[ sm->m_sequence ].bbmax[0];
	maxs[1] = pseqdesc[ sm->m_sequence ].bbmax[1];
	maxs[2] = pseqdesc[ sm->m_sequence ].bbmax[2];
}



void sm_GetSequenceInfo(struct studio_model *sm, float *pflFrameRate, float *pflGroundSpeed)
{
	struct studio_seqdesc	*pseqdesc;

	pseqdesc = (struct studio_seqdesc *)((uint8_t *)sm->m_pstudiohdr + sm->m_pstudiohdr->seqindex) + (int)sm->m_sequence;

	if (pseqdesc->numframes > 1)
	{
		*pflFrameRate = 256 * pseqdesc->fps / (pseqdesc->numframes - 1);
		*pflGroundSpeed = sqrt(pseqdesc->linearmovement[0]*pseqdesc->linearmovement[0]+ pseqdesc->linearmovement[1]*pseqdesc->linearmovement[1]+ pseqdesc->linearmovement[2]*pseqdesc->linearmovement[2]);
		*pflGroundSpeed = *pflGroundSpeed * pseqdesc->fps / (pseqdesc->numframes - 1);
	}
	else
	{
		*pflFrameRate = 256.0;
		*pflGroundSpeed = 0.0;
	}
}



float sm_SetController(struct studio_model *sm, int iController, float flValue)
{
	if (!sm->m_pstudiohdr)
		return 0.0f;

	struct studio_bonecontroller	*pbonecontroller = (struct studio_bonecontroller *)((uint8_t *)sm->m_pstudiohdr + sm->m_pstudiohdr->bonecontrollerindex);

	// find first controller that matches the index
	int i;
	for (i = 0; i < sm->m_pstudiohdr->numbonecontrollers; i++, pbonecontroller++)
	{
		if (pbonecontroller->index == iController)
			break;
	}
	if (i >= sm->m_pstudiohdr->numbonecontrollers)
		return flValue;

	// wrap 0..360 if it's a rotational controller
	if (pbonecontroller->type & (STUDIO_XR | STUDIO_YR | STUDIO_ZR))
	{
		// ugly hack, invert value if end < start
		if (pbonecontroller->end < pbonecontroller->start)
			flValue = -flValue;

		// does the controller not wrap?
		if (pbonecontroller->start + 359.0 >= pbonecontroller->end)
		{
			if (flValue > ((pbonecontroller->start + pbonecontroller->end) / 2.0) + 180)
				flValue = flValue - 360;
			if (flValue < ((pbonecontroller->start + pbonecontroller->end) / 2.0) - 180)
				flValue = flValue + 360;
		}
		else
		{
			if (flValue > 360)
				flValue = flValue - (int)(flValue / 360.0) * 360.0;
			else if (flValue < 0)
				flValue = flValue + (int)((flValue / -360.0) + 1) * 360.0;
		}
	}

	int setting = (int) (255 * (flValue - pbonecontroller->start) /
	(pbonecontroller->end - pbonecontroller->start));

	if (setting < 0) setting = 0;
	if (setting > 255) setting = 255;
	sm->m_controller[iController] = setting;

	return setting * (1.0 / 255.0) * (pbonecontroller->end - pbonecontroller->start) + pbonecontroller->start;
}


float sm_SetMouth(struct studio_model *sm, float flValue)
{
	int i;

	if (!sm->m_pstudiohdr)
		return 0.0f;

	struct studio_bonecontroller	*pbonecontroller = (struct studio_bonecontroller *)((uint8_t *)sm->m_pstudiohdr + sm->m_pstudiohdr->bonecontrollerindex);

	// find first controller that matches the mouth
	for (i = 0; i < sm->m_pstudiohdr->numbonecontrollers; i++, pbonecontroller++)
	{
		if (pbonecontroller->index == 4)
			break;
	}

	// wrap 0..360 if it's a rotational controller
	if (pbonecontroller->type & (STUDIO_XR | STUDIO_YR | STUDIO_ZR))
	{
		// ugly hack, invert value if end < start
		if (pbonecontroller->end < pbonecontroller->start)
			flValue = -flValue;

		// does the controller not wrap?
		if (pbonecontroller->start + 359.0 >= pbonecontroller->end)
		{
			if (flValue > ((pbonecontroller->start + pbonecontroller->end) / 2.0) + 180)
				flValue = flValue - 360;
			if (flValue < ((pbonecontroller->start + pbonecontroller->end) / 2.0) - 180)
				flValue = flValue + 360;
		}
		else
		{
			if (flValue > 360)
				flValue = flValue - (int)(flValue / 360.0) * 360.0;
			else if (flValue < 0)
				flValue = flValue + (int)((flValue / -360.0) + 1) * 360.0;
		}
	}

	int setting = (int) (64 * (flValue - pbonecontroller->start) / (pbonecontroller->end - pbonecontroller->start));

	if (setting < 0) setting = 0;
	if (setting > 64) setting = 64;
	sm->m_mouth = setting;

	return setting * (1.0 / 64.0) * (pbonecontroller->end - pbonecontroller->start) + pbonecontroller->start;
}


float sm_SetBlending(struct studio_model *sm, int iBlender, float flValue)
{
	struct studio_seqdesc	*pseqdesc;

	if (!sm->m_pstudiohdr)
		return 0.0f;

	pseqdesc = (struct studio_seqdesc *)((uint8_t *)sm->m_pstudiohdr + sm->m_pstudiohdr->seqindex) + (int)sm->m_sequence;

	if (pseqdesc->blendtype[iBlender] == 0)
		return flValue;

	if (pseqdesc->blendtype[iBlender] & (STUDIO_XR | STUDIO_YR | STUDIO_ZR))
	{
		// ugly hack, invert value if end < start
		if (pseqdesc->blendend[iBlender] < pseqdesc->blendstart[iBlender])
			flValue = -flValue;

		// does the controller not wrap?
		if (pseqdesc->blendstart[iBlender] + 359.0 >= pseqdesc->blendend[iBlender])
		{
			if (flValue > ((pseqdesc->blendstart[iBlender] + pseqdesc->blendend[iBlender]) / 2.0) + 180)
				flValue = flValue - 360;
			if (flValue < ((pseqdesc->blendstart[iBlender] + pseqdesc->blendend[iBlender]) / 2.0) - 180)
				flValue = flValue + 360;
		}
	}

	int setting = (int) (255 * (flValue - pseqdesc->blendstart[iBlender]) / (pseqdesc->blendend[iBlender] - pseqdesc->blendstart[iBlender]));

	if (setting < 0) setting = 0;
	if (setting > 255) setting = 255;

	sm->m_blending[iBlender] = setting;

	return setting * (1.0 / 255.0) * (pseqdesc->blendend[iBlender] - pseqdesc->blendstart[iBlender]) + pseqdesc->blendstart[iBlender];
}



int sm_SetBodygroup(struct studio_model *sm, int iGroup, int iValue)
{
	if (!sm->m_pstudiohdr)
		return 0;

	if (iGroup > sm->m_pstudiohdr->numbodyparts)
		return -1;

	struct studio_bodypart *pbodypart = (struct studio_bodypart *)((uint8_t *)sm->m_pstudiohdr + sm->m_pstudiohdr->bodypartindex) + iGroup;

	int iCurrent = (sm->m_bodynum / pbodypart->base) % pbodypart->nummodels;

	if (iValue >= pbodypart->nummodels)
		return iCurrent;

	sm->m_bodynum = (sm->m_bodynum - (iCurrent * pbodypart->base) + (iValue * pbodypart->base));

	return iValue;
}


int sm_SetSkin(struct studio_model *sm, int iValue)
{
	if (!sm->m_pstudiohdr)
		return 0;

	if (iValue >= sm->m_pstudiohdr->numskinfamilies)
	{
		return sm->m_skinnum;
	}

	sm->m_skinnum = iValue;

	return iValue;
}



void sm_scaleMeshes(struct studio_model *sm, float scale)
{
	if (!sm->m_pstudiohdr)
		return;

	int i, j, k;

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

void sm_scaleBones(struct studio_model *sm, float scale)
{
	int i, j;
	if (!sm->m_pstudiohdr)
		return;

	struct studio_bone *pbones = (struct studio_bone *) ((uint8_t *) sm->m_pstudiohdr + sm->m_pstudiohdr->boneindex);
	for (i = 0; i < sm->m_pstudiohdr->numbones; i++)
	{
		for (j = 0; j < 3; j++)
		{
			pbones[i].value[j] *= scale;
			pbones[i].scale[j] *= scale;
		}
	}	
}
