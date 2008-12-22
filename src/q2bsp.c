/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*
* QuakeII BSP map file loader.
*
* TODO
*  o Use multi-texture rendering to improve performance
*  o Use higher res textures with shaders
*  o Frustum / Bounding Box culling
*  o Simple gravity / physics model
*  o Load submodels
*  o Load portals
*  o Lots of tidying up and bugfixes
*/
#include <blackbloc.h>
#include <gfile.h>
#include <teximage.h>
#include <q2wal.h>
#include <q2bsp.h>
#include <client.h>

static struct bsp_mtexinfo *mtex;
static struct bsp_vertex *mvert;
static struct bsp_medge *medge;
static struct bsp_mplane *mplane;
static struct bsp_msurface *msurface;
static struct bsp_msurface **marksurface;
static struct bsp_mleaf *mleaf;
static struct bsp_mnode *mnode;
static struct bsp_vis *visofs;
static int *msurfedge;
static int nummarksurfaces;
static int numsurfaces;
static int numsurfedges;
static int numplanes;
static int numleafs;
static int numnodes;
static int numedges;
static unsigned char *map_visibility;
static unsigned char *vis;
static unsigned char *lightdata;

#define BLOCK_WIDTH 128
#define BLOCK_HEIGHT 128
#define LIGHTMAP_BYTES 4
#define GL_LIGHTMAP_FORMAT GL_RGBA

static int current_lightmap_texture;
static unsigned char lightmap_buffer[4*BLOCK_WIDTH*BLOCK_HEIGHT];
static int allocated[BLOCK_WIDTH];
static unsigned int lm_textures;
static float s_blocklights[34*34*3];

static void R_BuildLightMap(struct bsp_msurface *surf, unsigned char *dest, int stride);

static void lm_upload_block(void)
{
	int texture;

	texture = lm_textures + current_lightmap_texture;

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, BLOCK_WIDTH, BLOCK_HEIGHT,
			0, GL_LIGHTMAP_FORMAT, GL_UNSIGNED_BYTE, lightmap_buffer);

	current_lightmap_texture++;
}

static void lm_init_block(void)
{
	memset(allocated, 0, sizeof(allocated));
}

static int lm_alloc_block(int w, int h, int *x, int *y)
{
	int i, j;
	int best, best2;

	best = BLOCK_HEIGHT;

	for(i=0; i<BLOCK_WIDTH-w; i++) {
		best2 = 0;
		for(j=0; j<w; j++) {
			if ( allocated[i+j] >= best )
				break;
			if ( allocated[i+j] > best2 )
				best2 = allocated[i+j];
		}
		if ( j == w ) {
			*x = i;
			*y = best = best2;
		}
	}

	if ( best + h > BLOCK_HEIGHT )
		return 0;

	for(i=0; i<w; i++)
		allocated[*x + i] = best + h;

	return 1;
}

static void build_lightmap(struct bsp_msurface *surf)
{
	int smax, tmax;
	unsigned char *base;

	if ( surf->flags & (SURF_DRAWSKY|SURF_DRAWTURB) ) {
		surf->lightmaptexturenum = -1;
		return;
	}

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;

	if ( !lm_alloc_block(smax, tmax,
			&surf->light_s, &surf->light_t) ) {
		lm_upload_block();
		lm_init_block();
		if ( !lm_alloc_block(smax, tmax,
				&surf->light_s, &surf->light_t) ) {
			surf->lightmaptexturenum = -1;
			return;
		}

	}

	surf->lightmaptexturenum = current_lightmap_texture;

	base = lightmap_buffer;
	base += (surf->light_t * BLOCK_WIDTH + surf->light_s) *
			LIGHTMAP_BYTES;

	R_BuildLightMap(surf, base, BLOCK_WIDTH*LIGHTMAP_BYTES);
}

static void build_polygon(struct bsp_msurface *fa)
{
	int i, lindex, lnumverts;
	struct bsp_medge *r_pedge;
	int vertpage;
	float *vec;
	float s, t;
	struct bsp_poly *poly;

	lnumverts = fa->numedges;
	vertpage = 0;

	poly = malloc(sizeof(*poly) + (lnumverts) * VERTEXSIZE*sizeof(float));
	poly->next = fa->polys;
	poly->flags = fa->flags;
	fa->polys = poly;
	poly->numverts = lnumverts;

	for (i=0 ; i<lnumverts ; i++)
	{
		lindex = msurfedge[fa->firstedge + i];

		if (lindex > 0) {
			r_pedge = medge + lindex;
			vec = mvert[r_pedge->v[0]].point;
		}else{
			r_pedge = medge - lindex;
			vec = mvert[r_pedge->v[1]].point;
		}

		s = v_dotproduct(vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->image->mipmap[0].width;

		t = v_dotproduct(vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->image->mipmap[0].height;

		poly->verts[i][0] = vec[0];
		poly->verts[i][1] = vec[1];
		poly->verts[i][2] = vec[2];
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;

		s = v_dotproduct(vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s * 16;
		s += 8;
		s /= BLOCK_WIDTH*16;

		t = v_dotproduct(vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t * 16;
		t += 8;
		t /= BLOCK_WIDTH*16;

		poly->verts[i][5] = s;
		poly->verts[i][6] = t;
	}

	poly->numverts = lnumverts;
}

static void CalcSurfaceExtents(struct bsp_msurface *s)
{
	float	mins[2], maxs[2], val;
	int		i,j, e;
	struct bsp_vertex *v;
	struct bsp_mtexinfo *tex;
	int		bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = s->texinfo;
	
	for (i=0 ; i<s->numedges ; i++)
	{
		e = msurfedge[s->firstedge+i];
		if (e >= 0)
			v = mvert + medge[e].v[0];
		else
			v = mvert + medge[-e].v[1];
		
		for (j=0 ; j<2 ; j++)
		{
			val = v->point[0] * tex->vecs[j][0] + 
				v->point[1] * tex->vecs[j][1] +
				v->point[2] * tex->vecs[j][2] +
				tex->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i=0 ; i<2 ; i++)
	{	
		bmins[i] = floor(mins[i]/16);
		bmaxs[i] = ceil(maxs[i]/16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;
	}
}

static int q2bsp_submodels(void *data, int len)
{
	struct bsp_model *in;
	struct bsp_model *out;
	int count;

	in=data;
	count = len / sizeof(*in);

	if ( len % sizeof(*in) ) {
		con_printf("q2bsp: funny model lump size\n");
		return -1;
	}

	if ( count <= 0 ) {
		con_printf("q2bsp: bad model count\n");
		return -1;
	}

	if ( !(out=malloc(count * sizeof(*out))) ) {
		con_printf("q2bsp: model oom\n");
		return -1;
	}

	return 0;
}

static void node_set_parent(struct bsp_mnode *n, struct bsp_mnode *p)
{
	n->parent = p;
	if ( n->contents != -1 )
		return;
	node_set_parent(n->children[0], n);
	node_set_parent(n->children[1], n);
}

static int q2bsp_nodes(void *data, int len)
{
	struct bsp_node *in;
	struct bsp_mnode *out;
	int i, j, p, count;

	in=data;
	count = len / sizeof(*in);

	if ( len % sizeof(*in) ) {
		con_printf("q2bsp: funny node lump size\n");
		return -1;
	}

	if ( count <= 0 ) {
		con_printf("q2bsp: bad node count\n");
		return -1;
	}

	if ( !(out=malloc(count * sizeof(*out))) ) {
		con_printf("q2bsp: node oom\n");
		return -1;
	}

	mnode=out;
	numnodes=count;

	for(i=0; i<count; i++, in++, out++) {
		out->mins[0] = (short)le_16(in->mins[1]);
		out->mins[1] = (short)le_16(in->mins[2]);
		out->mins[2] = (short)le_16(in->mins[0]);
		out->maxs[0] = (short)le_16(in->maxs[1]);
		out->maxs[1] = (short)le_16(in->maxs[2]);
		out->maxs[2] = (short)le_16(in->maxs[0]);

		p = le_32(in->planenum);
		out->plane = mplane + p;

		out->firstsurface = le_16(in->firstface);
		out->numsurfaces = le_16(in->numfaces);
		out->contents = -1; /* diferrenciate from leafs */
		out->visframe = 0;

		for(j=0; j<2; j++) {
			p = le_32(in->children[j]);
			if ( p>= 0 )
				out->children[j] = mnode + p;
			else
				out->children[j] = (struct bsp_mnode *)mleaf + (-1 - p);
		}
	}

	node_set_parent(mnode, NULL);

	return 0;
}

static int q2bsp_leafs(void *data, int len)
{
	struct bsp_leaf *in;
	struct bsp_mleaf *out;
	int i, p, count;

	in=data;
	count = len / sizeof(*in);

	if ( len % sizeof(*in) ) {
		con_printf("q2bsp: funny leaf lump size\n");
		return -1;
	}

	if ( count <= 0 ) {
		con_printf("q2bsp: bad leaf count\n");
		return -1;
	}

	if ( !(out=malloc(count * sizeof(*out))) ) {
		con_printf("q2bsp: leaf oom\n");
		return -1;
	}

	mleaf = out;
	numleafs = count;

	for(i=0; i<count; i++, in++, out++) {
		out->mins[0] = (short)le_16(in->mins[1]);
		out->mins[1] = (short)le_16(in->mins[2]);
		out->mins[2] = (short)le_16(in->mins[0]);
		out->maxs[0] = (short)le_16(in->maxs[1]);
		out->maxs[1] = (short)le_16(in->maxs[2]);
		out->maxs[2] = (short)le_16(in->maxs[0]);

		p = (int)le_32(in->contents);
		out->contents = p;

		out->cluster = (short)le_16(in->cluster);
		out->area = (short)le_16(in->area);
		out->firstmarksurface = marksurface +
			le_16(in->firstleafface);
		out->nummarksurfaces = le_16(in->numleaffaces);
		out->visframe = 0;
	}

	return 0;
}

static int q2bsp_visibility(void *data, int len)
{
	struct bsp_vis *out;
	struct bsp_vis *in = data;
	int num = le_32(in->numclusters);
	int i;

	map_visibility = data;

	if ( !(out=malloc(num * sizeof(*out))) ) {
		con_printf("q2bsp: vis oom\n");
		return -1;
	}

	if ( !(vis=malloc(num>>3)) ) {
		con_printf("q2bsp: vis oom 2\n");
		return -1;
	}

	out->numclusters = num;

	for(i=0; i<num; i++) {
		out->bitofs[i][DVIS_PVS] = le_32(in->bitofs[i][DVIS_PVS]);
		out->bitofs[i][DVIS_PHS] = le_32(in->bitofs[i][DVIS_PHS]);
	}

	visofs = out;

	return 0;
}

static int q2bsp_marksurfaces(void *data, int len)
{
	int i, j, count;
	short *in;
	struct bsp_msurface **out;

	in=data;
	count = len / sizeof(*in);

	if ( len % sizeof(*in) ) {
		con_printf("q2bsp: funny leaf face lump size\n");
		return -1;
	}

	if ( count <= 0 ) {
		con_printf("q2bsp: bad leaf face count\n");
		return -1;
	}

	if ( !(out=malloc(count * sizeof(*out))) ) {
		con_printf("q2bsp: leaf face oom\n");
		return -1;
	}

	marksurface = out;
	nummarksurfaces = count;

	for(i=0; i<count; i++) {
		j = (short)le_16(in[i]);
		if ( j<0 || j>= numsurfaces ) {
			con_printf("q2bsp: bad leaf face surface number\n");
			return -1;
		}
		out[i] = msurface + j;
	}

	return 0;
}

static int q2bsp_faces(void *data, int len)
{
	struct bsp_face *in;
	struct bsp_msurface *out;
	int count, surfnum;
	int planenum, side;
	int i;

	in=data;
	count = len / sizeof(*in);

	if ( len % sizeof(*in) ) {
		con_printf("q2bsp: funny face lump size\n");
		return -1;
	}

	if ( count <= 0 ) {
		con_printf("q2bsp: bad face count\n");
		return -1;
	}

	if ( !(out=malloc(count * sizeof(*out))) ) {
		con_printf("q2bsp: face oom\n");
		return -1;
	}

	msurface = out;
	numsurfaces = count;

	current_lightmap_texture = 1;
	lm_textures = 1024;
	lm_init_block();

	for(surfnum=0; surfnum<count; surfnum++, in++, out++) {
		out->firstedge = (int)le_32(in->firstedge);
		out->numedges = (short)le_16(in->numedges);
		out->flags = 0;
		out->polys = NULL;
		out->visframe = 0;

		planenum = le_16(in->planenum);
		side = (short)le_16(in->side);
		if ( side )
			out->flags |= SURF_PLANEBACK;

		/* XXX: check both of these */
		out->plane = mplane + planenum;
		out->texinfo = mtex + le_16(in->texinfo);

		CalcSurfaceExtents(out);

		for(i=0; i<MAXLIGHTMAPS; i++)
			out->styles[i] = in->styles[i];

		i = le_32(in->lightofs);
		if ( i == -1 )
			out->samples = NULL;
		else
			out->samples = lightdata + i;

		/* Create lightmaps */
		if ( !(out->texinfo->flags & (SURF_SKY|SURF_TRANS33|SURF_TRANS66|SURF_WARP)) ) {
			build_lightmap(out);
		}else{
			out->lightmaptexturenum = -1;
		}

		/* Create Polygon */
		build_polygon(out);
	}

	lm_upload_block();
	return 0;
}

static int q2bsp_planes(void *data, int len)
{
	struct bsp_plane *in;
	struct bsp_mplane *out;
	int i, count;
	int bits;

	in=data;
	count = len / sizeof(*in);

	if ( len % sizeof(*in) ) {
		con_printf("q2bsp: funny plane lump size\n");
		return -1;
	}

	if ( count <= 0 ) {
		con_printf("q2bsp: bad plane count\n");
		return -1;
	}

	if ( !(out=malloc(count * sizeof(*out))) ) {
		con_printf("q2bsp: plane oom\n");
		return -1;
	}

	mplane = out;
	numplanes = count;

	for(i=0; i<count; i++,in++,out++) {
		bits=0;

		out->normal[0] = le_float(in->normal[1]);
		out->normal[1] = le_float(in->normal[2]);
		out->normal[2] = le_float(in->normal[0]);

		out->normal[W] = 0;
		out->dist = le_float(in->dist);
		out->type = le_32(in->type);
		out->signbits = bits;

		switch(out->type) {
		case PLANE_X:
			out->type = 2;
			break;
		case PLANE_Y:
			out->type = 0;
			break;
		case PLANE_Z:
			out->type = 1;
			break;
		}
	}

	return 0;
}

static int q2bsp_lighting(void *data, int len)
{
	lightdata = data;
	return 0;
}

static int q2bsp_surfedges(void *data, int len)
{
	int *in, *out;
	int i, count;

	in=data;
	count = len / sizeof(*in);

	if ( len % sizeof(*in) ) {
		con_printf("q2bsp: funny surfedge lump size\n");
		return -1;
	}

	if ( count <= 0 ) {
		con_printf("q2bsp: bad surfedge count\n");
		return -1;
	}

	if ( !(out=malloc(count * sizeof(*out))) ) {
		con_printf("q2bsp: surfedge oom\n");
		return -1;
	}

	msurfedge=out;
	numsurfedges=count;

	for(i=0; i<count; i++, out++, in++)
		*out = le_32(*in);

	return 0;
}

static int q2bsp_edges(void *data, int len)
{
	struct bsp_edge *in;
	struct bsp_medge *out;
	int i, count;

	in=data;
	count = len / sizeof(*in);

	if ( len % sizeof(*in) ) {
		con_printf("q2bsp: funny edge lump size\n");
		return -1;
	}

	if ( count <= 0 ) {
		con_printf("q2bsp: bad edge count\n");
		return -1;
	}

	if ( !(out=malloc(count * sizeof(*out))) ) {
		con_printf("q2bsp: edge oom\n");
		return -1;
	}

	medge = out;
	numedges = count;

	for(i=0; i<count; i++,in++,out++) {
		out->v[0] = (short)le_16(in->v[0]);
		out->v[1] = (short)le_16(in->v[1]);
	}

	return 0;
}

static int q2bsp_vertexes(void *data, int len)
{
	struct bsp_vertex *in, *out;
	int i, count;

	in=data;
	count = len / sizeof(*in);

	if ( len % sizeof(*in) ) {
		con_printf("q2bsp: funny vertex lump size\n");
		return -1;
	}

	if ( count <= 0 ) {
		con_printf("q2bsp: bad vertex count\n");
		return -1;
	}

	if ( !(out=malloc(count * sizeof(*out))) ) {
		con_printf("q2bsp: vertex oom\n");
		return -1;
	}

	mvert = out;

	for(i=0; i<count; i++,in++,out++) {
		out->point[0] = le_float(in->point[1]);
		out->point[1] = le_float(in->point[2]);
		out->point[2] = le_float(in->point[0]);
	}

	return 0;
}

static int q2bsp_texinfo(void *data, int len)
{
	struct bsp_texinfo *in;
	struct bsp_mtexinfo *out, *step;
	int count, i, j, next;

	in=data;
	count = len / sizeof(*in);

	if ( len % sizeof(*in) ) {
		con_printf("q2bsp: funny texinfo lump size\n");
		return -1;
	}

	if ( count <= 0 ) {
		con_printf("q2bsp: bad texinfo count\n");
		return -1;
	}

	if ( !(out=malloc(count * sizeof(*out))) ) {
		con_printf("q2bsp: texinfo oom\n");
		return -1;
	}

	mtex = out;

	for(i=0; i < count; i++, in++, out++) {
		for(j=0; j<2; j++) {
			out->vecs[j][0] = le_float(in->vecs[j][1]);
			out->vecs[j][1] = le_float(in->vecs[j][2]);
			out->vecs[j][2] = le_float(in->vecs[j][0]);
			out->vecs[j][3] = le_float(in->vecs[j][3]);
		}

		out->flags = le_32(in->flags);

		/* XXX: Needs checking */
		next = le_32(in->nexttexinfo);
		if (next > 0)
			out->next = mtex + next;
		else
			out->next = NULL;

		out->image = q2wal_get(in->texture);
		if ( !out->image ) {
			con_printf("q2bsp: texture %s not found\n",
				in->texture);
			return -1;
		}
	}

	/* Count animation frames */
	for(i=0; i<count; i++) {
		out=&mtex[i];
		out->numframes=1;
		for(step=out->next; step && step != out; step=step->next)
			out->numframes++;
	}

	return 0;
}

int q2bsp_load(char *name)
{
	struct gfile f;
	struct bsp_header hdr;
	uint32_t *tmp;
	int i;

	if ( !game_open(&f, name) ) {
		con_printf("bsp: %s not found\n", name);
		goto err;
	}

	if ( f.f_len < sizeof(struct bsp_header) ) {
		con_printf("bsp: %s: too small for header\n", name);
		goto err_close;
	}

	/* Byteswap header */
	memcpy(&hdr, f.f_ptr, sizeof(hdr));
	tmp=(uint32_t *)&hdr;
	for(i=0; i<sizeof(hdr)/sizeof(*tmp); i++) {
		tmp[i]=le_32(tmp[i]);
	}

	/* Check header */
	if ( hdr.ident != IDBSPHEADER ||
		hdr.version != BSPVERSION ) {
		con_printf("bsp: %s: bad ident or version (v%u)\n",
			name, hdr.version);
		goto err_close;
	}

	/* Check all lumps are in the file */
	for(i=0; i<HEADER_LUMPS; i++) {
		if ( hdr.lumps[i].ofs + hdr.lumps[i].len > f.f_len ) {
			con_printf("bsp: %s: lump %i is fucked up\n",
				name, i);
			goto err_close;
		}
	}

	q2bsp_vertexes(f.f_ptr + hdr.lumps[LUMP_VERTEXES].ofs,
			hdr.lumps[LUMP_VERTEXES].len);
	q2bsp_edges(f.f_ptr + hdr.lumps[LUMP_EDGES].ofs,
			hdr.lumps[LUMP_EDGES].len);
	q2bsp_surfedges(f.f_ptr + hdr.lumps[LUMP_SURFEDGES].ofs,
			hdr.lumps[LUMP_SURFEDGES].len);
	q2bsp_lighting(f.f_ptr + hdr.lumps[LUMP_LIGHTING].ofs,
			hdr.lumps[LUMP_LIGHTING].len);
	q2bsp_planes(f.f_ptr + hdr.lumps[LUMP_PLANES].ofs,
			hdr.lumps[LUMP_PLANES].len);
	q2bsp_texinfo(f.f_ptr + hdr.lumps[LUMP_TEXINFO].ofs,
			hdr.lumps[LUMP_TEXINFO].len);
	q2bsp_faces(f.f_ptr + hdr.lumps[LUMP_FACES].ofs,
			hdr.lumps[LUMP_FACES].len);
	q2bsp_marksurfaces(f.f_ptr + hdr.lumps[LUMP_LEAFFACES].ofs,
			hdr.lumps[LUMP_LEAFFACES].len);
	q2bsp_visibility(f.f_ptr + hdr.lumps[LUMP_VISIBILITY].ofs,
			hdr.lumps[LUMP_VISIBILITY].len);
	q2bsp_leafs(f.f_ptr + hdr.lumps[LUMP_LEAFS].ofs,
			hdr.lumps[LUMP_LEAFS].len);
	q2bsp_nodes(f.f_ptr + hdr.lumps[LUMP_NODES].ofs,
			hdr.lumps[LUMP_NODES].len);
	q2bsp_submodels(f.f_ptr + hdr.lumps[LUMP_MODELS].ofs,
			hdr.lumps[LUMP_MODELS].len);

	con_printf("bsp: %s loaded OK\n", name);
	return 0;

err_close:
	game_close(&f);
err:
	return -1;
}

static void q2bsp_surfrender(struct bsp_msurface *s)
{
	int i;
	float *v;
	struct bsp_poly *p=s->polys;
	int lm = (s->lightmaptexturenum != -1);
	int lmtex = s->lightmaptexturenum;

	/* Draw lightmap */
#if 1
	if ( lm ) {
		glBindTexture(GL_TEXTURE_2D, lm_textures + lmtex);
		glBegin(GL_POLYGON);
		v = p->verts[0];
		for(i = 0; i < p->numverts; i++, v += VERTEXSIZE) {
			glTexCoord2f(v[5],v[6]);
			glVertex3fv(v);
		}
		glEnd();

		glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendFunc(GL_ZERO, GL_SRC_COLOR);
	}
#endif

#if 1
	img_bind(s->texinfo->image);
	glBegin(GL_POLYGON);
	v=p->verts[0];
	for(i=0; i<p->numverts; i++, v+=VERTEXSIZE) {
		glTexCoord2f(v[3],v[4]);
		glVertex3fv(v);
	}
	glEnd();
#endif

	glDisable(GL_BLEND);
	glDepthMask(1);
}

static int cull_box(vector_t mins, vector_t maxs)
{
	return 0;
}

static void q2bsp_recurse(struct bsp_mnode *n, vector_t org, int visframe)
{
	struct bsp_mplane *plane;
	struct bsp_msurface *surf;
	int side, sidebit;
	scalar_t dot;
	int c;

	if ( n->contents == CONTENTS_SOLID )
		return;

	if ( n->visframe != visframe )
		return;

	/* TODO: Bounding Box check */
	if ( cull_box(n->mins, n->maxs) )
		return;

	/* leaf node, render */
	if ( n->contents != -1 ) {
		struct bsp_mleaf *leaf;
		struct bsp_msurface **mark;

		leaf = (struct bsp_mleaf *)n;

		/* XXX: Check for doors */

		mark = leaf->firstmarksurface;
		c = leaf->nummarksurfaces;
		if ( !c )
			return;

		do {
			(*mark)->visframe = visframe;
			mark++;
		}while(--c);

		return;
	}

	plane = n->plane;
	switch (plane->type) {
	case PLANE_X:
		dot = org[X] - plane->dist;
		break;
	case PLANE_Y:
		dot = org[Y] - plane->dist;
		break;
	case PLANE_Z:
		dot = org[Z] - plane->dist;
		break;
	default:
		dot = v_dotproduct(org, plane->normal) - plane->dist;
	}

	if ( dot < 0 ) {
		side = 1;
		sidebit = SURF_PLANEBACK;
	}else{
		side = 0;
		sidebit = 0;
	}

	q2bsp_recurse(n->children[side], org, visframe);

	for(c=n->numsurfaces, surf=msurface + n->firstsurface; c; c--, surf++) {
		if ( surf->visframe != visframe )
			continue;

		if ( (surf->flags & SURF_PLANEBACK) != sidebit )
			continue;

		q2bsp_surfrender(surf);
	}
	
	q2bsp_recurse(n->children[!side], org, visframe);
}

static void decompress_vis(int ofs)
{
	unsigned int c, v;
	unsigned int b;

	memset(vis, 0, visofs->numclusters>>3);

	/* Mark all leaves to be rendered */
	for(c=0,v=ofs; c<visofs->numclusters; v++) {
		if ( map_visibility[v] == 0 ) {
			v++;
			c += 8*map_visibility[v];
		}else{
			for(b=1; b & 0xff; b<<=1, c++) {
				if ( !(map_visibility[v] & b) )
					continue;

				vis[c>>3] |= (1<<(c&7));
			}
		}
	}
}

static void mark_leafs(int newc, int visframe)
{
	struct bsp_mleaf *leaf;
	struct bsp_mnode *node;
	int i, marked=0;
	int cluster;

	for(i=0, leaf=mleaf; i<numleafs; i++, leaf++) {
		cluster = leaf->cluster;

		if ( cluster == -1 )
			continue;

		if ( vis[cluster>>3] & (1<<(cluster&7)) ) {
			node = (struct bsp_mnode *)leaf;
			do {
				if ( node->visframe == visframe )
					break;
				node->visframe = visframe;
				node = node->parent;
				marked++;
			}while(node);
		}
	}
}

static void mark_all(int visframe)
{
	int i;

	for(i=0; i<numleafs; i++)
		mleaf[i].visframe = visframe;
	for(i=0; i<numnodes; i++)
		mnode[i].visframe = visframe;
}


void q2bsp_render(void)
{
	struct bsp_mnode *node = mnode;
	struct bsp_mplane *plane;
	static int oldc=-1, newc=-1;
	static int visframe=0;
	vector_t org;
	scalar_t d;

	if ( !node )
		return;

	/* Obtain eye coordinates */
	v_copy(org, me.origin);
	v_add(org, org, me.viewoffset);

	/* Traverse the BSP tree and find which leaf we are in,
	 * we then know what cluster we are in */
	for(;;) {
		if ( node->contents != -1 )
			break;

		plane = node->plane;
		d = v_dotproduct(org, plane->normal) - plane->dist;
		if ( d > 0 )
			node = node->children[0];
		else
			node = node->children[1];
	}

	oldc = newc;
	newc = ((struct bsp_mleaf *)node)->cluster;

	/* Recalculate vis if view cluster changed */
	if ( oldc != newc || visframe == 0 ) {
		visframe++;

		if ( newc == -1 ) {
			mark_all(visframe);
		}else{
			decompress_vis(visofs->bitofs[newc][DVIS_PVS]);
			vis[newc>>3] |= (1<<(newc&7));
			mark_leafs(newc, visframe);
		}
	}

	/* Traverse the BSP tree and render */
	glCullFace(GL_FRONT);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glBlendFunc(GL_ZERO, GL_SRC_COLOR);
	glDisable(GL_BLEND);
	q2bsp_recurse(mnode, org, visframe);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
}

static void R_BuildLightMap(struct bsp_msurface *surf, unsigned char *dest, int stride)
{
	int			smax, tmax;
	int			r, g, b, a, max;
	int			i, j, size;
	unsigned char *lightmap;
	float		scale[4];
	int			nummaps;
	float		*bl;

	if ( surf->texinfo->flags & (SURF_SKY|SURF_TRANS33|SURF_TRANS66|SURF_WARP) ) {
		con_printf("R_BuildLightMap called for non-lit surface\n");
		return;
	}

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	size = smax*tmax;
	if (size > (sizeof(s_blocklights)>>4) ) {
		con_printf("Bad s_blocklights size\n");
		return;
	}

// set to full bright if no light data
	if (!surf->samples)
	{
		for (i=0 ; i<size*3 ; i++)
			s_blocklights[i] = 255;
		goto store;
	}

	// count the # of maps
	for ( nummaps = 0 ; nummaps < MAXLIGHTMAPS && surf->styles[nummaps] != 255 ;
		 nummaps++)
		;

	lightmap = surf->samples;

	// add all the lightmaps
	if ( nummaps == 1 )
	{
		int maps;

		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			bl = s_blocklights;

			for (i=0 ; i<3 ; i++)
				scale[i] = 1;

			if ( scale[0] == 1.0F &&
				 scale[1] == 1.0F &&
				 scale[2] == 1.0F )
			{
				for (i=0 ; i<size ; i++, bl+=3)
				{
					bl[0] = lightmap[i*3+0];
					bl[1] = lightmap[i*3+1];
					bl[2] = lightmap[i*3+2];
				}
			}
			else
			{
				for (i=0 ; i<size ; i++, bl+=3)
				{
					bl[0] = lightmap[i*3+0] * scale[0];
					bl[1] = lightmap[i*3+1] * scale[1];
					bl[2] = lightmap[i*3+2] * scale[2];
				}
			}
			lightmap += size*3;		// skip to next lightmap
		}
	}
	else
	{
		int maps;

		memset( s_blocklights, 0, sizeof( s_blocklights[0] ) * size * 3 );

		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			bl = s_blocklights;

			for (i=0 ; i<3 ; i++)
				scale[i] = 1;

			if ( scale[0] == 1.0F &&
				 scale[1] == 1.0F &&
				 scale[2] == 1.0F )
			{
				for (i=0 ; i<size ; i++, bl+=3 )
				{
					bl[0] += lightmap[i*3+0];
					bl[1] += lightmap[i*3+1];
					bl[2] += lightmap[i*3+2];
				}
			}
			else
			{
				for (i=0 ; i<size ; i++, bl+=3)
				{
					bl[0] += lightmap[i*3+0] * scale[0];
					bl[1] += lightmap[i*3+1] * scale[1];
					bl[2] += lightmap[i*3+2] * scale[2];
				}
			}
			lightmap += size*3;		// skip to next lightmap
		}
	}

// put into texture format
store:
	stride -= (smax<<2);
	bl = s_blocklights;

	for (i=0 ; i<tmax ; i++, dest += stride)
	{
		for (j=0 ; j<smax ; j++)
		{
			
			r = (long) bl[0];
			g = (long) bl[1];
			b = (long) bl[2];

			// catch negative lights
			if (r < 0)
				r = 0;
			if (g < 0)
				g = 0;
			if (b < 0)
				b = 0;

			/*
			** determine the brightest of the three color components
			*/
			if (r > g)
				max = r;
			else
				max = g;
			if (b > max)
				max = b;

			/*
			** alpha is ONLY used for the mono lightmap case.  For this reason
			** we set it to the brightest of the color components so that 
			** things don't get too dim.
			*/
			a = max;

			/*
			** rescale all the color components if the intensity of the greatest
			** channel exceeds 1.0
			*/
			if (max > 255)
			{
				float t = 255.0F / max;

				r = r*t;
				g = g*t;
				b = b*t;
				a = a*t;
			}

			dest[0] = r;
			dest[1] = g;
			dest[2] = b;
			dest[3] = a;

			bl += 3;
			dest += 4;
		}
	}
}
