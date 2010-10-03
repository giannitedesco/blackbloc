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
#include <stdio.h>

#include <blackbloc/blackbloc.h>
#include <blackbloc/gfile.h>
#include <blackbloc/client.h>
#include <blackbloc/tex.h>
#include <blackbloc/img/q2wal.h>
#include <blackbloc/img/tga.h>
#include <blackbloc/map/q2bsp.h>

#include "q2bsp.h"

static void R_BuildLightMap(struct _q2bsp *bsp, struct bsp_msurface *surf,
				unsigned char *dest, int stride);

static void lm_upload_block(struct _q2bsp *map)
{
	int texture;

	texture = map->lm_textures + map->current_lightmap_texture;

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, BLOCK_WIDTH, BLOCK_HEIGHT,
			0, GL_LIGHTMAP_FORMAT, GL_UNSIGNED_BYTE,
			map->lightmap_buffer);

	map->current_lightmap_texture++;
}

static void lm_init_block(struct _q2bsp *map)
{
	memset(map->allocated, 0, sizeof(map->allocated));
}

static int lm_alloc_block(struct _q2bsp *map, int w, int h, int *x, int *y)
{
	int i, j;
	int best, best2;

	best = BLOCK_HEIGHT;

	for(i=0; i<BLOCK_WIDTH-w; i++) {
		best2 = 0;
		for(j=0; j<w; j++) {
			if ( map->allocated[i+j] >= best )
				break;
			if ( map->allocated[i+j] > best2 )
				best2 = map->allocated[i+j];
		}
		if ( j == w ) {
			*x = i;
			*y = best = best2;
		}
	}

	if ( best + h > BLOCK_HEIGHT )
		return 0;

	for(i=0; i<w; i++)
		map->allocated[*x + i] = best + h;

	return 1;
}

static void build_lightmap(struct _q2bsp *map, struct bsp_msurface *surf)
{
	int smax, tmax;
	unsigned char *base;

	if ( surf->flags & (SURF_DRAWSKY|SURF_DRAWTURB) ) {
		surf->lightmaptexturenum = -1;
		return;
	}

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;

	if ( !lm_alloc_block(map, smax, tmax,
			&surf->light_s, &surf->light_t) ) {
		lm_upload_block(map);
		lm_init_block(map);
		if ( !lm_alloc_block(map, smax, tmax,
				&surf->light_s, &surf->light_t) ) {
			surf->lightmaptexturenum = -1;
			return;
		}

	}

	surf->lightmaptexturenum = map->current_lightmap_texture;

	base = map->lightmap_buffer;
	base += (surf->light_t * BLOCK_WIDTH + surf->light_s) *
			LIGHTMAP_BYTES;

	R_BuildLightMap(map, surf, base, BLOCK_WIDTH*LIGHTMAP_BYTES);
}

static void build_polygon(struct _q2bsp *map, struct bsp_msurface *fa)
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
		lindex = map->msurfedge[fa->firstedge + i];

		if (lindex > 0) {
			r_pedge = map->medge + lindex;
			vec = map->mvert[r_pedge->v[0]].point;
		}else{
			r_pedge = map->medge - lindex;
			vec = map->mvert[r_pedge->v[1]].point;
		}

		s = v_dotproduct(vec, fa->texinfo->vecs[0]) +
					fa->texinfo->vecs[0][3];

		t = v_dotproduct(vec, fa->texinfo->vecs[1]) +
					fa->texinfo->vecs[1][3];

		if ( tex_height(fa->texinfo->image) ==
			tex_width(fa->texinfo->image) ) {
			t /= 64.0f;
			s /= 64.0f;
		}else{
			t /= tex_height(fa->texinfo->image);
			s /= tex_width(fa->texinfo->image);
		}

		poly->verts[i][0] = vec[0];
		poly->verts[i][1] = vec[1];
		poly->verts[i][2] = vec[2];
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;

		s = v_dotproduct(vec, fa->texinfo->vecs[0]) +
					fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s * 16;
		s += 8;
		s /= BLOCK_WIDTH*16;

		t = v_dotproduct(vec, fa->texinfo->vecs[1]) +
					fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t * 16;
		t += 8;
		t /= BLOCK_WIDTH*16;

		poly->verts[i][5] = s;
		poly->verts[i][6] = t;
	}

	poly->numverts = lnumverts;
}

static void CalcSurfaceExtents(struct _q2bsp *map, struct bsp_msurface *s)
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
		e = map->msurfedge[s->firstedge+i];
		if (e >= 0)
			v = map->mvert + map->medge[e].v[0];
		else
			v = map->mvert + map->medge[-e].v[1];
		
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

static int q2bsp_submodels(struct _q2bsp *map, const void *data, uint32_t len)
{
	const struct bsp_model *in;
	struct bsp_model *out;
	int count;

	in = data;
	count = len / sizeof(*in);

	if ( len % sizeof(*in) ) {
		con_printf("q2bsp: funny model lump size\n");
		return 0;
	}

	if ( count <= 0 ) {
		con_printf("q2bsp: bad model count\n");
		return 0;
	}

	if ( !(out=malloc(count * sizeof(*out))) ) {
		con_printf("q2bsp: model oom\n");
		return 0;
	}

	/* TODO */

	return 1;
}

static void node_set_parent(struct bsp_mnode *n, struct bsp_mnode *p)
{
	n->parent = p;
	if ( n->contents != -1 )
		return;
	node_set_parent(n->children[0], n);
	node_set_parent(n->children[1], n);
}

static int q2bsp_nodes(struct _q2bsp *map, const void *data, uint32_t len)
{
	const struct bsp_node *in;
	struct bsp_mnode *out;
	int i, j, p, count;

	in = data;
	count = len / sizeof(*in);

	if ( len % sizeof(*in) ) {
		con_printf("q2bsp: funny node lump size\n");
		return 0;
	}

	if ( count <= 0 ) {
		con_printf("q2bsp: bad node count\n");
		return 0;
	}

	if ( !(out=malloc(count * sizeof(*out))) ) {
		con_printf("q2bsp: node oom\n");
		return 0;
	}

	map->mnode = out;
	map->numnodes = count;

	for(i=0; i<count; i++, in++, out++) {
		out->mins[0] = (short)le16toh(in->mins[1]);
		out->mins[1] = (short)le16toh(in->mins[2]);
		out->mins[2] = (short)le16toh(in->mins[0]);
		out->maxs[0] = (short)le16toh(in->maxs[1]);
		out->maxs[1] = (short)le16toh(in->maxs[2]);
		out->maxs[2] = (short)le16toh(in->maxs[0]);

		p = le_32(in->planenum);
		out->plane = map->mplane + p;

		out->firstsurface = le16toh(in->firstface);
		out->numsurfaces = le16toh(in->numfaces);
		out->contents = -1; /* diferrenciate from leafs */
		out->visframe = 0;

		for(j=0; j<2; j++) {
			p = le_32(in->children[j]);
			if ( p>= 0 )
				out->children[j] = map->mnode + p;
			else
				out->children[j] =
					(struct bsp_mnode *)map->mleaf +
								(-1 - p);
		}
	}

	node_set_parent(map->mnode, NULL);

	return 1;
}

static int q2bsp_leafs(struct _q2bsp *map, const void *data, uint32_t len)
{
	const struct bsp_leaf *in;
	struct bsp_mleaf *out;
	int i, p, count;

	in = data;
	count = len / sizeof(*in);

	if ( len % sizeof(*in) ) {
		con_printf("q2bsp: funny leaf lump size\n");
		return 0;
	}

	if ( count <= 0 ) {
		con_printf("q2bsp: bad leaf count\n");
		return 0;
	}

	if ( !(out=malloc(count * sizeof(*out))) ) {
		con_printf("q2bsp: leaf oom\n");
		return 0;
	}

	map->mleaf = out;
	map->numleafs = count;

	for(i=0; i<count; i++, in++, out++) {
		out->mins[0] = (short)le16toh(in->mins[1]);
		out->mins[1] = (short)le16toh(in->mins[2]);
		out->mins[2] = (short)le16toh(in->mins[0]);
		out->maxs[0] = (short)le16toh(in->maxs[1]);
		out->maxs[1] = (short)le16toh(in->maxs[2]);
		out->maxs[2] = (short)le16toh(in->maxs[0]);

		p = (int)le_32(in->contents);
		out->contents = p;

		out->cluster = (short)le16toh(in->cluster);
		out->area = (short)le16toh(in->area);
		out->firstmarksurface = map->marksurface +
			le16toh(in->firstleafface);
		out->nummarksurfaces = le16toh(in->numleaffaces);
		out->visframe = 0;
	}

	return 1;
}

static int q2bsp_visibility(struct _q2bsp *map, const void *data, uint32_t len)
{
	const struct bsp_vis *in = data;
	struct bsp_vis *out;
	int num = le_32(in->numclusters);
	int i;

	map->map_visibility = data;

	if ( !(out=malloc(num * sizeof(*out))) ) {
		con_printf("q2bsp: vis oom\n");
		return 0;
	}

	if ( !(map->vis=malloc(num>>3)) ) {
		con_printf("q2bsp: vis oom 2\n");
		return 0;
	}

	out->numclusters = num;

	for(i=0; i<num; i++) {
		out->bitofs[i][DVIS_PVS] = le_32(in->bitofs[i][DVIS_PVS]);
		out->bitofs[i][DVIS_PHS] = le_32(in->bitofs[i][DVIS_PHS]);
	}

	map->visofs = out;

	return 1;
}

static int q2bsp_marksurfaces(struct _q2bsp *map, const void *data, uint32_t len)
{
	int i, j, count;
	const short *in;
	struct bsp_msurface **out;

	in = data;
	count = len / sizeof(*in);

	if ( len % sizeof(*in) ) {
		con_printf("q2bsp: funny leaf face lump size\n");
		return 0;
	}

	if ( count <= 0 ) {
		con_printf("q2bsp: bad leaf face count\n");
		return 0;
	}

	if ( !(out=malloc(count * sizeof(*out))) ) {
		con_printf("q2bsp: leaf face oom\n");
		return 0;
	}

	map->marksurface = out;
	map->nummarksurfaces = count;

	for(i=0; i<count; i++) {
		j = (short)le16toh(in[i]);
		if ( j < 0 || j >= map->numsurfaces ) {
			con_printf("q2bsp: bad leaf face surface number\n");
			return 0;
		}
		out[i] = map->msurface + j;
	}

	return 1;
}

static int q2bsp_faces(struct _q2bsp *map, const void *data, uint32_t len)
{
	const struct bsp_face *in;
	struct bsp_msurface *out;
	int count, surfnum;
	int planenum, side;
	int i;

	in = data;
	count = len / sizeof(*in);

	if ( len % sizeof(*in) ) {
		con_printf("q2bsp: funny face lump size\n");
		return 0;
	}

	if ( count <= 0 ) {
		con_printf("q2bsp: bad face count\n");
		return 0;
	}

	if ( !(out=malloc(count * sizeof(*out))) ) {
		con_printf("q2bsp: face oom\n");
		return 0;
	}

	map->msurface = out;
	map->numsurfaces = count;

	map->current_lightmap_texture = 1;
	map->lm_textures = 1024;
	lm_init_block(map);

	for(surfnum=0; surfnum<count; surfnum++, in++, out++) {
		out->firstedge = (int)le_32(in->firstedge);
		out->numedges = (short)le16toh(in->numedges);
		out->flags = 0;
		out->polys = NULL;
		out->visframe = 0;

		planenum = le16toh(in->planenum);
		side = (short)le16toh(in->side);
		if ( side )
			out->flags |= SURF_PLANEBACK;

		/* XXX: check both of these */
		out->plane = map->mplane + planenum;
		out->texinfo = map->mtex + le16toh(in->texinfo);

		CalcSurfaceExtents(map, out);

		for(i=0; i<MAXLIGHTMAPS; i++)
			out->styles[i] = in->styles[i];

		i = le_32(in->lightofs);
		if ( i == -1 )
			out->samples = NULL;
		else
			out->samples = (void *)(map->lightdata + i);

		/* Create lightmaps */
		if ( !(out->texinfo->flags & (SURF_SKY|SURF_TRANS33|SURF_TRANS66|SURF_WARP)) ) {
			build_lightmap(map, out);
		}else{
			out->lightmaptexturenum = -1;
		}

		/* Create Polygon */
		build_polygon(map, out);
	}

	lm_upload_block(map);
	return 1;
}

static int q2bsp_planes(struct _q2bsp *map, const void *data, uint32_t len)
{
	const struct bsp_plane *in;
	struct bsp_mplane *out;
	int i, count;
	int bits;

	in = data;
	count = len / sizeof(*in);

	if ( len % sizeof(*in) ) {
		con_printf("q2bsp: funny plane lump size\n");
		return 0;
	}

	if ( count <= 0 ) {
		con_printf("q2bsp: bad plane count\n");
		return 0;
	}

	if ( !(out=malloc(count * sizeof(*out))) ) {
		con_printf("q2bsp: plane oom\n");
		return 0;
	}

	map->mplane = out;
	map->numplanes = count;

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

	return 1;
}

static int q2bsp_lighting(struct _q2bsp *map, const void *data, uint32_t len)
{
	map->lightdata = data;
	return 1;
}

static int q2bsp_surfedges(struct _q2bsp *map, const void *data, uint32_t len)
{
	const int *in;
	int i, count, *out;

	in = data;
	count = len / sizeof(*in);

	if ( len % sizeof(*in) ) {
		con_printf("q2bsp: funny surfedge lump size\n");
		return 0;
	}

	if ( count <= 0 ) {
		con_printf("q2bsp: bad surfedge count\n");
		return 0;
	}

	if ( !(out=malloc(count * sizeof(*out))) ) {
		con_printf("q2bsp: surfedge oom\n");
		return 0;
	}

	map->msurfedge = out;
	map->numsurfedges = count;

	for(i = 0; i < count; i++, out++, in++)
		*out = le32toh(*in);

	return 1;
}

static int q2bsp_edges(struct _q2bsp *map, const void *data, uint32_t len)
{
	const struct bsp_edge *in;
	struct bsp_medge *out;
	int i, count;

	in = data;
	count = len / sizeof(*in);

	if ( len % sizeof(*in) ) {
		con_printf("q2bsp: funny edge lump size\n");
		return 0;
	}

	if ( count <= 0 ) {
		con_printf("q2bsp: bad edge count\n");
		return 0;
	}

	if ( !(out=malloc(count * sizeof(*out))) ) {
		con_printf("q2bsp: edge oom\n");
		return 0;
	}

	map->medge = out;
	map->numedges = count;

	for(i=0; i<count; i++,in++,out++) {
		out->v[0] = (short)le16toh(in->v[0]);
		out->v[1] = (short)le16toh(in->v[1]);
	}

	return 1;
}

static int q2bsp_vertexes(struct _q2bsp *map, const void *data, int len)
{
	const struct bsp_vertex *in;
	struct bsp_vertex *out;
	int i, count;

	in = data;
	count = len / sizeof(*in);

	if ( len % sizeof(*in) ) {
		con_printf("q2bsp: funny vertex lump size\n");
		return 0;
	}

	if ( count <= 0 ) {
		con_printf("q2bsp: bad vertex count\n");
		return 0;
	}

	out = malloc(count * sizeof(*out));
	if ( out == NULL ) {
		con_printf("q2bsp: vertex oom\n");
		return 0;
	}

	map->mvert = out;

	for(i=0; i<count; i++,in++,out++) {
		out->point[0] = le_float(in->point[1]);
		out->point[1] = le_float(in->point[2]);
		out->point[2] = le_float(in->point[0]);
	}

	return 1;
}

static texture_t get_texture(const char *name)
{
	texture_t ret;
	size_t len = strlen(name);
	char fn[9 + len + 5];

	snprintf(fn, sizeof(fn), "textures/%s.tga", name);
	ret = tga_get_by_name(fn);
	if ( ret )
		return ret;

	return NULL; //return q2wal_get(name);
}

static int q2bsp_texinfo(struct _q2bsp *map, const void *data, uint32_t len)
{
	const struct bsp_texinfo *in;
	struct bsp_mtexinfo *out, *step;
	int count, i, j, next;

	in = data;
	count = len / sizeof(*in);

	if ( len % sizeof(*in) ) {
		con_printf("q2bsp: funny texinfo lump size\n");
		return 0;
	}

	if ( count <= 0 ) {
		con_printf("q2bsp: bad texinfo count\n");
		return 0;
	}

	if ( !(out=malloc(count * sizeof(*out))) ) {
		con_printf("q2bsp: texinfo oom\n");
		return 0;
	}

	map->mtex = out;

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
			out->next = map->mtex + next;
		else
			out->next = NULL;

		out->image = get_texture(in->texture);
		if ( !out->image ) {
			con_printf("q2bsp: texture %s not found\n",
				in->texture);
			return 0;
		}
		map->num_mtex++;
	}

	/* Count animation frames */
	for(i=0; i < count; i++) {
		out = &map->mtex[i];
		out->numframes = 1;
		for(step=out->next; step && step != out; step=step->next)
			out->numframes++;
	}

	return 1;
}

static void do_free(struct _q2bsp *map)
{
	unsigned int i;
	free(map->mtex);
	free(map->mvert);
	free(map->medge);
	free(map->mplane);
	free(map->msurface);
	free(map->marksurface);
	free(map->mleaf);
	free(map->mnode);
	free(map->visofs);
	free(map->msurfedge);
	free(map->vis);
	for(i = 0; i < map->num_mtex; i++)
		tex_put(map->mtex[i].image);
	free(map);
}

void q2bsp_free(q2bsp_t map)
{
	if ( map )
		do_free(map);
}

q2bsp_t q2bsp_load(const char *name)
{
	struct _q2bsp *map;
	struct gfile f;
	struct bsp_header hdr;
	uint32_t *tmp;
	unsigned int i;
	int ret;

	map = calloc(1, sizeof(*map));
	if ( NULL == map ) {
		con_printf("bsp: %s: %s\n", name, get_err());
		goto err;
	}

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
	tmp = (uint32_t *)&hdr;
	for(i = 0; i < sizeof(hdr) / sizeof(*tmp); i++)
		tmp[i] = le32toh(tmp[i]);

	/* Check header */
	if ( hdr.ident != IDBSPHEADER ||
		hdr.version != BSPVERSION ) {
		con_printf("bsp: %s: bad ident or version (v%u)\n",
			name, hdr.version);
		goto err_close;
	}

	/* Check all lumps are in the file */
	for(i = 0; i < HEADER_LUMPS; i++) {
		if ( hdr.lumps[i].ofs + hdr.lumps[i].len > f.f_len ) {
			con_printf("bsp: %s: lump %i is fucked up\n",
				name, i);
			goto err_close;
		}
	}

	ret = q2bsp_vertexes(map, f.f_ptr + hdr.lumps[LUMP_VERTEXES].ofs,
			hdr.lumps[LUMP_VERTEXES].len);
	if ( !ret )
		goto err_close;

	ret = q2bsp_edges(map, f.f_ptr + hdr.lumps[LUMP_EDGES].ofs,
			hdr.lumps[LUMP_EDGES].len);
	if ( !ret )
		goto err_close;

	ret = q2bsp_surfedges(map, f.f_ptr + hdr.lumps[LUMP_SURFEDGES].ofs,
			hdr.lumps[LUMP_SURFEDGES].len);
	if ( !ret )
		goto err_close;

	ret = q2bsp_lighting(map, f.f_ptr + hdr.lumps[LUMP_LIGHTING].ofs,
			hdr.lumps[LUMP_LIGHTING].len);
	if ( !ret )
		goto err_close;

	ret = q2bsp_planes(map, f.f_ptr + hdr.lumps[LUMP_PLANES].ofs,
			hdr.lumps[LUMP_PLANES].len);
	if ( !ret )
		goto err_close;

	ret = q2bsp_texinfo(map, f.f_ptr + hdr.lumps[LUMP_TEXINFO].ofs,
			hdr.lumps[LUMP_TEXINFO].len);
	if ( !ret )
		goto err_close;

	ret = q2bsp_faces(map, f.f_ptr + hdr.lumps[LUMP_FACES].ofs,
			hdr.lumps[LUMP_FACES].len);
	if ( !ret )
		goto err_close;

	ret = q2bsp_marksurfaces(map, f.f_ptr + hdr.lumps[LUMP_LEAFFACES].ofs,
			hdr.lumps[LUMP_LEAFFACES].len);
	if ( !ret )
		goto err_close;

	ret = q2bsp_visibility(map, f.f_ptr + hdr.lumps[LUMP_VISIBILITY].ofs,
			hdr.lumps[LUMP_VISIBILITY].len);
	if ( !ret )
		goto err_close;

	ret = q2bsp_leafs(map, f.f_ptr + hdr.lumps[LUMP_LEAFS].ofs,
			hdr.lumps[LUMP_LEAFS].len);
	if ( !ret )
		goto err_close;

	ret = q2bsp_nodes(map, f.f_ptr + hdr.lumps[LUMP_NODES].ofs,
			hdr.lumps[LUMP_NODES].len);
	if ( !ret )
		goto err_close;

	ret = q2bsp_submodels(map, f.f_ptr + hdr.lumps[LUMP_MODELS].ofs,
			hdr.lumps[LUMP_MODELS].len);
	if ( !ret )
		goto err_close;


	con_printf("bsp: %s loaded OK\n", name);
	return map;

err_close:
	game_close(&f);
	do_free(map);
err:
	return NULL;
}

static void q2bsp_surfrender(struct _q2bsp *map, struct bsp_msurface *s)
{
	int i;
	float *v;
	struct bsp_poly *p=s->polys;
	int lm = (s->lightmaptexturenum != -1);
	int lmtex = s->lightmaptexturenum;

	/* Draw lightmap */
#if 1
	if ( lm ) {
		glBindTexture(GL_TEXTURE_2D, map->lm_textures + lmtex);
		glBegin(GL_POLYGON);
		v = p->verts[0];
		for(i = 0; i < p->numverts; i++, v += VERTEXSIZE) {
			glTexCoord2f(v[5],v[6]);
			glVertex3fv(v);
		}
		glEnd();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendFunc(GL_ZERO, GL_SRC_COLOR);
	}
#endif

#if 1
	tex_bind(s->texinfo->image);
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

static void q2bsp_recurse(struct _q2bsp *map, struct bsp_mnode *n,
				vector_t org, int visframe)
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

	q2bsp_recurse(map, n->children[side], org, visframe);

	for(c=n->numsurfaces, surf = map->msurface + n->firstsurface;
			c; c--, surf++) {
		if ( surf->visframe != visframe )
			continue;

		if ( (surf->flags & SURF_PLANEBACK) != sidebit )
			continue;

		q2bsp_surfrender(map, surf);
	}
	
	q2bsp_recurse(map, n->children[!side], org, visframe);
}

static void decompress_vis(struct _q2bsp *map, int ofs)
{
	unsigned int c, v;
	unsigned int b;

	memset(map->vis, 0, map->visofs->numclusters >> 3);

	/* Mark all leaves to be rendered */
	for(c = 0, v = ofs; c < map->visofs->numclusters; v++) {
		if ( map->map_visibility[v] == 0 ) {
			c += 8 * map->map_visibility[++v];
		}else{
			for(b = 1; b & 0xff; b <<= 1, c++) {
				if ( (map->map_visibility[v] & b) == 0 )
					continue;

				map->vis[c >> 3] |= (1 << (c & 7));
			}
		}
	}
}

static void mark_leafs(struct _q2bsp *map, int newc, int visframe)
{
	struct bsp_mleaf *leaf;
	struct bsp_mnode *node;
	int i, marked=0;
	int cluster;

	for(i=0, leaf = map->mleaf; i < map->numleafs; i++, leaf++) {
		cluster = leaf->cluster;

		if ( cluster == -1 )
			continue;

		if ( map->vis[cluster >> 3] & (1 << (cluster & 7)) ) {
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

static void mark_all(struct _q2bsp *map, int visframe)
{
	int i;

	for(i=0; i < map->numleafs; i++)
		map->mleaf[i].visframe = visframe;
	for(i=0; i < map->numnodes; i++)
		map->mnode[i].visframe = visframe;
}


void q2bsp_render(q2bsp_t map)
{
	struct bsp_mnode *node = map->mnode;
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
			mark_all(map, visframe);
		}else{
			decompress_vis(map,
					map->visofs->bitofs[newc][DVIS_PVS]);
			map->vis[newc >> 3] |= (1 << (newc & 7));
			mark_leafs(map, newc, visframe);
		}
	}

	/* Traverse the BSP tree and render */
	glCullFace(GL_FRONT);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glBlendFunc(GL_ZERO, GL_SRC_COLOR);
	glDisable(GL_BLEND);
	q2bsp_recurse(map, map->mnode, org, visframe);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
}

static void R_BuildLightMap(struct _q2bsp *map, struct bsp_msurface *surf,
				unsigned char *dest, int stride)
{
	unsigned int smax, tmax;
	unsigned int r, g, b, a, max;
	unsigned int i, j, size;
	unsigned char *lightmap;
	float scale[4];
	int nummaps;
	float *bl;

	if ( surf->texinfo->flags &
			(SURF_SKY|SURF_TRANS33|SURF_TRANS66|SURF_WARP) ) {
		con_printf("R_BuildLightMap called for non-lit surface\n");
		return;
	}

	smax = (surf->extents[0] >> 4) + 1;
	tmax = (surf->extents[1] >> 4) + 1;
	size = smax * tmax;
	if (size > (sizeof(map->s_blocklights) >> 4) ) {
		con_printf("Bad map->s_blocklights size\n");
		return;
	}

// set to full bright if no light data
	if (!surf->samples)
	{
		for (i=0 ; i<size*3 ; i++)
			map->s_blocklights[i] = 255;
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
			bl = map->s_blocklights;

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

		memset( map->s_blocklights, 0, sizeof( map->s_blocklights[0] ) * size * 3 );

		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			bl = map->s_blocklights;

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
	bl = map->s_blocklights;

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
