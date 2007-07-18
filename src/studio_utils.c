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
#include <bytesex.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <studio_model.h>
#include <render_settings.h>

#define swap32(var) do { var = le_32(var); } while(0);
#define swap16(var) do { var = le_16(var); } while(0);
#define swapfloat(var) do { var = le_float(var); } while(0);
#define swapvec3(var) do { le_vec3(var); } while(0);
static void le_vec3(vec3_t x)
{
	swapfloat(x[0]);
	swapfloat(x[1]);
	swapfloat(x[2]);
}

//extern float			g_bonetransform[MAXSTUDIOBONES][3][4];

ViewerSettings g_viewerSettings;
struct studio_model g_studioModel;
static unsigned int bFilterTextures = 1;


////////////////////////////////////////////////////////////////////////

static int g_texnum = 30;



static void sm_UploadTexture(struct studio_model *sm,
			struct studio_texture *ptexture,
			uint8_t *data, uint8_t *pal, int name)
{
	// unsigned *in, int inwidth, int inheight, unsigned *out,  int outwidth, int outheight;
	int		i, j;
	int		row1[256], row2[256], col1[256], col2[256];
	uint8_t	*pix1, *pix2, *pix3, *pix4;
	uint8_t	*tex, *out;
	int32_t outwidth;
	int32_t outheight;

	// convert texture to power of 2
	for (outwidth = 1; outwidth < ptexture->width; outwidth <<= 1)
		;

	//outwidth >>= 1;
	if (outwidth > 256)
		outwidth = 256;

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

/* Byte swap the studio file, validating everything as we go.
 * FIXME: Still not validating everything which is used to deref an array,
 * eg: bone indexes > numbones and so on...
 */
static int swap_model(const char *fn, struct studio_hdr *h, size_t len)
{
	uint8_t *buf = (uint8_t *)h;
	uint8_t *end = h + len;
	uint16_t *skin;
	uint32_t i;

	if ( len < sizeof(*h) )
		goto err;

	swap32(h->id);
	swap32(h->version);
	swap32(h->length);
	swapvec3(h->eyeposition);
	swapvec3(h->min);
	swapvec3(h->max);
	swapvec3(h->bbmin);
	swapvec3(h->bbmax);
	swap32(h->flags);
	swap32(h->numbones);
	swap32(h->boneindex);
	swap32(h->numbonecontrollers);
	swap32(h->bonecontrollerindex);
	swap32(h->numhitboxes);
	swap32(h->hitboxindex);
	swap32(h->numseq);
	swap32(h->seqindex);
	swap32(h->numseqgroups);
	swap32(h->seqgroupindex);
	swap32(h->numtextures);
	swap32(h->textureindex);
	swap32(h->numskinref);
	swap32(h->numskinfamilies);
	swap32(h->skinindex);
	swap32(h->numbodyparts);
	swap32(h->bodypartindex);
	swap32(h->numattachments);
	swap32(h->attachmentindex);
	swap32(h->soundtable);
	swap32(h->soundindex);
	swap32(h->soundgroups);
	swap32(h->numtransitions);
	swap32(h->transitionindex);

	if ( h->id != STUDIO_IDST && h->id != STUDIO_IDSQ ) {
		con_printf("studio: %s: Bad magic\n", fn);
		return 0;
	}

	if ( h->length != len )
		goto err;

	con_printf("studio: %s: %s\n", fn,
		(h->id == STUDIO_IDST) ? "IDST" : "IDSQ");
	con_printf(" o version %u, %uKB - %s\n",
		h->version, h->length/1024, h->name);

	if ( h->id == STUDIO_IDSQ )
		return 1;

	con_printf(" o %u bones, %u controllers\n",
			h->numbones, h->numbonecontrollers);
	con_printf(" o %u sequences, %u groups\n",
		h->numseq, h->numseqgroups);
	con_printf(" o %u hit boxes\n", h->numhitboxes);
	con_printf(" o %u textures\n", h->numtextures);
	con_printf(" o %u skins %u families\n",
		h->numskinref, h->numskinfamilies);
	con_printf(" o %u bodyparts, %u attachments\n",
		h->numbodyparts, h->numattachments);

	if ( h->numbones ) {
		struct studio_bone *b = buf + h->boneindex;
		if ( b + h->numbones > end )
			goto err;
		for(i = 0; i < h->numbones; i++) {
			uint32_t j;
			swap32(b[i].parent);
			swap32(b[i].flags);
			for(j = 0; j < 6; j++) {
				swap32(b[i].bonecontroller[j]);
				swapfloat(b[i].value[j]);
				swapfloat(b[i].scale[j]);
			}
			con_printf("  bone[%.2u]: parent=%i "
				"flags=0x%x: %s\n", i, 
				b[i].parent, b[i].flags, b[i].name);
		}
	}

	if ( h->numbonecontrollers ) {
		struct studio_bonecontroller *c = buf + h->bonecontrollerindex;
		if ( c + h->numbones > end )
			goto err;
		for(i = 0; i < h->numbonecontrollers; i++) {
			swap32(c[i].bone);
			swap32(c[i].type);
			swapfloat(c[i].start);
			swapfloat(c[i].end);
			swap32(c[i].rest);
			swap32(c[i].index);
			con_printf("  controller[%.2u] bone=%i type=%i "
					"index=%u\n",
					i, c[i].bone, c[i].index, c[i].index);
		}
	}

	if ( h->numhitboxes ) {
		struct studio_bbox *x = buf + h->hitboxindex;
		if ( x + h->numhitboxes > end )
			goto err;
		for(i = 0; i < h->numhitboxes; i++) {
			swap32(x[i].bone);
			swap32(x[i].group);
			swapvec3(x[i].bbmin);
			swapvec3(x[i].bbmax);
		}
	}

	if ( h->numseq ) {
		struct studio_seqdesc *s = buf + h->seqindex;
		unsigned int j;
		if ( s + h->numseq > end )
			goto err;
		for(i = 0; i < h->numseq; i++) {
			swapfloat(s[i].fps);
			swap32(s[i].flags);
			swap32(s[i].activity);
			swap32(s[i].actweight);
			swap32(s[i].numevents);
			swap32(s[i].eventindex);
			swap32(s[i].numframes);
			swap32(s[i].numpivots);
			swap32(s[i].pivotindex);
			swap32(s[i].motiontype);
			swap32(s[i].motionbone);
			swapvec3(s[i].linearmovement);
			swap32(s[i].automoveposindex);
			swap32(s[i].automoveangleindex);
			swapvec3(s[i].bbmin);
			swapvec3(s[i].bbmax);
			swap32(s[i].numblends);
			swap32(s[i].animindex);
			for(j = 0; j < 2; j++) {
				swap32(s[i].blendtype[j]);
				swapfloat(s[i].blendstart[j]);
				swapfloat(s[i].blendend[j]);
			}
			swap32(s[i].blendparent);
			swap32(s[i].seqgroup);
			swap32(s[i].entrynode);
			swap32(s[i].exitnode);
			swap32(s[i].nodeflags);
			swap32(s[i].nextseq);

			con_printf("  seq[%.2u]: grp%.2u %.1ffps %.1fs %s\n", 
				i, s[i].seqgroup, s[i].fps,
				(float)s[i].numframes / s[i].fps,
				s[i].label);

			if ( s[i].seqgroup >= h->numseqgroups )
				goto err;
		}
	}

	if ( h->numseqgroups ) {
		struct studio_seqgroup *g = buf + h->seqgroupindex;
		if ( g + h->numtextures > end )
			goto err;
		for(i = 0; i < h->numseqgroups; i++) {
			swap32(g[i].data);
			swap32(g[i].cache);
			con_printf("  group[%.2u] %s cache=%p data=%i: %s\n",
				i, g[i].label, g[i].cache,
				g[i].data, g[i].name);
			if ( g[i].cache )
				g[i].cache = NULL;
		}
	}

	if ( h->numtextures ) {
		struct studio_texture *t = buf + h->textureindex;
		if ( t + h->numtextures > end )
			goto err;
		for(i = 0; i < h->numtextures; i++) {
			swap32(t[i].flags);
			swap32(t[i].width);
			swap32(t[i].height);
			swap32(t[i].index);
			/* FIXME: Check pixel data */
			con_printf("  tex[%.2u]: flags=0x%x %ux%u: %s\n", i,
				t[i].flags,
				t[i].width,
				t[i].height,
				t[i].name);
		}

	}

	skin = buf + h->skinindex;
	if ( skin + (h->numskinref * h->numskinfamilies) > end )
		goto err;
	for(i = 0; i < h->numskinfamilies; i++) {
		unsigned int j;
		con_printf("  skin[%.2u]:", i);
		for(j = 0; j < h->numskinref; j++) {
			swap16(skin[i*h->numskinref+j]);
			con_printf(" %u", skin[i*h->numskinref+j]);
		}
		con_printf("\n");
	}

	if ( h->numbodyparts ) {
		struct studio_bodypart *p = buf + h->bodypartindex;
		if ( p + h->numbodyparts > end )
			goto err;
		for(i = 0; i < h->numbodyparts; i++) {
			struct studio_dmodel *m;
			unsigned int j;

			swap32(p[i].nummodels);
			swap32(p[i].base);
			swap32(p[i].modelindex);
			con_printf("  bodypart[%.2u]: %u models b=%u %s\n",
				i, p[i].nummodels, p[i].base, p[i].name);

			m = buf + p[i].modelindex;
			if ( m + p[i].nummodels > end )
				goto err;

			for(j = 0; j < p[i].nummodels; j++) {
				struct studio_mesh *msh;
				unsigned int k;
				vec3_t *v;

				swap32(m[j].type);
				swap32(m[j].nummesh);
				swapfloat(m[j].boundingradius);
				swap32(m[j].meshindex);
				swap32(m[j].numverts);
				swap32(m[j].vertinfoindex);
				swap32(m[j].vertindex);
				swap32(m[j].numnorms);
				swap32(m[j].norminfoindex);
				swap32(m[j].normindex);
				swap32(m[j].numgroups);
				swap32(m[j].groupindex);
				con_printf("  x model[%.2u]: meshes=%u "
					"verts=%u tris=%u: %s\n", j, 
					m[j].nummesh,
					m[j].numverts,
					m[j].numnorms,
					m[j].name);

				v = buf + m[j].vertindex;
				if ( v + m[j].numverts > end )
					goto err;
				for(k = 0; k < m[j].numverts; k++)
					swapvec3(v[k]);

				v = buf + m[j].normindex;
				if ( v + m[j].numnorms > end )
					goto err;
				for(k = 0; k < m[j].numverts; k++)
					swapvec3(v[k]);

				msh = buf + m[j].meshindex;
				if ( msh + m[j].nummesh > end )
					goto err;

				for(k = 0; k < m[j].nummesh; k++) {
					swap32(msh[k].numtris);
					swap32(msh[k].triindex);
					swap32(msh[k].numnorms);
					swap32(msh[k].skinref);
					swap32(msh[k].normindex);
					con_printf("    - mesh[%u]: %u tris, %u norms skin %.2u\n",
						k,
						msh[k].numnorms,
						msh[k].numtris,
						msh[k].skinref);
					/* FIXME: validate tricmds */
				}
			}
		}
	}

	if ( h->numattachments ) {
		struct studio_attachment *a = buf + h->attachmentindex;
		if ( a + h->numattachments > end )
			goto err;
		for(i = 0; i < h->numattachments; i++) {
			unsigned int j;
			swap32(a[i].type);
			swap32(a[i].bone);
			swapvec3(a[i].org);
			for(j = 0; j < 3; j++)
				swapvec3(a[i].vectors[j]);
			con_printf("  attach[%.2u]: type=%u bone=%i %s\n",
				a[i].type, a[i].bone, a[i].name);
		}
	}

	return 1;
err:
	con_printf("studio: %s: corrupt header\n", fn);
	return 0;
}

/* Here we just load the data structures from the file, validate them and
 * then upload the textures
 */
struct studio_hdr *sm_LoadModel(struct studio_model *sm, char *modelname)
{
	FILE *fp;
	long size;
	void *buf;
	uint8_t	*pin;
	struct studio_hdr *h;
	struct studio_texture *ptexture;
	int i;

	if ( modelname == NULL )
		return NULL;

	// load the model
	fp = fopen(modelname, "rb");
	if ( fp == NULL) {
		con_printf("studio: %s: %s\n",
				modelname, load_err(NULL));
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	buf = malloc(size);
	if (buf == NULL ) {
		con_printf("studio: %s: malloc(): %s\n",
				modelname, load_err(NULL));
		fclose (fp);
		return NULL;
	}

	fread(buf, size, 1, fp);
	fclose(fp);

	pin = (uint8_t *)buf;
	h = (struct studio_hdr *)pin;

	/* Byte swap the main header */
	if ( !swap_model(modelname, h, size) ) {
		free(buf);
		return NULL;
	}

	if (sm->m_pstudiohdr == NULL && h->id != STUDIO_IDST) {
		con_printf("studio: %s: IDST expected\n", modelname);
		free (buf);
		return NULL;
	}

	ptexture = (struct studio_texture *)(pin + h->textureindex);
	if (h->textureindex > 0 && h->numtextures <= MAXSTUDIOSKINS) {
		int n = h->numtextures;
		for (i = 0; i < n; i++)
			sm_UploadTexture(sm, &ptexture[i],
				buf + ptexture[i].index,
				buf + ptexture[i].width *
					ptexture[i].height + ptexture[i].index,
				g_texnum++);
	}

	// UNDONE: free texture memory

	if ( sm->m_pstudiohdr == NULL )
		sm->m_pstudiohdr = pin;

	return (struct studio_hdr *)buf;
}


/* After already loading the file, we read in relevant data structures and
 * get ready for rendering
 */
int sm_PostLoadModel(struct studio_model *sm, char *modelname)
{
	int n, i;

	// preload textures
	if (sm->m_pstudiohdr->numtextures == 0) {
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
	}else{
		sm->m_ptexturehdr = sm->m_pstudiohdr;
		sm->m_owntexmodel = 0;
	}

	// preload animations
	for (i = 1; i < sm->m_pstudiohdr->numseqgroups; i++) {
		char fn[PATH_MAX];

		snprintf(fn, sizeof(fn) - 2, "%s", modelname);
		sprintf(fn + strlen(fn) - 4, "%02d.mdl", i);

		sm->m_panimhdr[i] = sm_LoadModel(sm, fn);
		if ( sm->m_panimhdr[i] == NULL ) {
			sm_FreeModel(sm);
			return 0;
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
	return 1;
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
