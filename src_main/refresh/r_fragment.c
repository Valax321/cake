/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

// r_fragment.c - fragment clipping against bsp

#include "r_local.h"

/*
=======================================================================

FRAGMENT CLIPPING

=======================================================================
*/

#define	SIDE_FRONT				0
#define	SIDE_BACK				1
#define	SIDE_ON					2

#define ON_EPSILON				0.1

#define MAX_FRAGMENT_POINTS		128
#define MAX_FRAGMENT_PLANES		6

static int				cm_numMarkPoints;
static int				cm_maxMarkPoints;
static vec3_t			*cm_markPoints;

static int				cm_numMarkFragments;
static int				cm_maxMarkFragments;
static markFragment_t	*cm_markFragments;

static cplane_t			cm_markPlanes[MAX_FRAGMENT_PLANES];

static int				cm_fragmentFrame;

// ripped from q2map
static int PlaneTypeForNormal (vec3_t normal)
{ 
	if (normal[0] == 1.0)  
		return 0;// PLANE_X;  
	if (normal[1] == 1.0)  
		return 1;// PLANE_Y;  
	if (normal[2] == 1.0)  
		return 2;// PLANE_Z; 
 	return 3;// PLANE_NON_AXIAL; 
} 

static float *worldVert (int i, msurface_t *surf)
{
	int e = r_worldmodel->surfedges[surf->firstedge + i];
	if (e >= 0)
		return &r_worldmodel->vertexes[r_worldmodel->edges[e].v[0]].position[0];
	else
		return &r_worldmodel->vertexes[r_worldmodel->edges[-e].v[1]].position[0];
}

/*
=================
R_ClipFragment
=================
*/
static void R_ClipFragment (int numPoints, vec4_t vecs, int stage, markFragment_t *mf)
{
	cplane_t	*plane;
	qboolean	front, back;
	vec4_t		newv[MAX_FRAGMENT_POINTS];
	float		*v, dist, dists[MAX_FRAGMENT_POINTS];
	int			newc, i, j, sides[MAX_FRAGMENT_POINTS];

	if (numPoints > MAX_FRAGMENT_POINTS - 2)
		VID_Error(ERR_DROP, "R_ClipFragment: MAX_FRAGMENT_POINTS hit");

	if (stage == MAX_FRAGMENT_PLANES)
	{
		// fully clipped
		if (numPoints > 2)
		{
			mf->numPoints = numPoints;
			mf->firstPoint = cm_numMarkPoints;
			
			if (cm_numMarkPoints + numPoints > cm_maxMarkPoints)
				numPoints = cm_maxMarkPoints - cm_numMarkPoints;

			for (i = 0, v = vecs; i < numPoints; i++, v += 4)
				VectorCopy(v, cm_markPoints[cm_numMarkPoints + i]);

			cm_numMarkPoints += numPoints;
		}
		return;
	}

	front = back = false;
	plane = &cm_markPlanes[stage];
	for (i = 0, v = vecs; i < numPoints; i++, v += 4)
	{
		if (plane->type < 3)
			dists[i] = dist = v[plane->type] - plane->dist;
		else
			dists[i] = dist = DotProduct (v, plane->normal) - plane->dist;

		if (dist > ON_EPSILON)
		{			
			front = true;
			sides[i] = SIDE_FRONT;
		}
		else if (dist < -ON_EPSILON)
		{
			back = true;
			sides[i] = SIDE_BACK;
		}
		else
		{
			sides[i] = SIDE_ON;
		}
	}

	if (!front)
		return; // not clipped

	// clip it
	dists[i] = dists[0];
	sides[i] = sides[0];
	VectorCopy (vecs, (vecs + (i * 4)));
	newc = 0;

	for (i = 0, v = vecs; i < numPoints; i++, v += 4)
	{
		switch (sides[i])
		{
			case SIDE_FRONT:
				VectorCopy(v, newv[newc]);
				newc++;
				break;
			case SIDE_BACK:
				break;
			case SIDE_ON:
				VectorCopy(v, newv[newc]);
				newc++;
				break;
		}

		if (sides[i] == SIDE_ON || sides[i + 1] == SIDE_ON || sides[i+1] == sides[i])
			continue;

		dist = dists[i] / (dists[i] - dists[i+1]);
		for (j = 0; j < 3; j++)
			newv[newc][j] = v[j] + dist * (v[j + 4] - v[j]);

		newc++;
	}

	// continue
	R_ClipFragment (newc, newv[0], stage + 1, mf);
}

/*
=================
R_ClipFragmentToSurface
=================
*/
static void R_ClipFragmentToSurface (msurface_t *surf, const vec3_t normal, mnode_t *node)
{
	int				i;
	vec4_t			verts[MAX_FRAGMENT_POINTS];
	markFragment_t	*mf;
	
	// clip as triangle fan
	for (i = 0; i < surf->numvertexes - 2; i++)
	{
		mf = &cm_markFragments[cm_numMarkFragments];
		mf->numPoints = 0;
		mf->node = node; // vis node
		mf->surf = surf;

		VectorCopy (worldVert(0, surf), verts[0]);
		VectorCopy (worldVert(i + 1, surf), verts[1]);
		VectorCopy (worldVert(i + 2, surf), verts[2]);

		R_ClipFragment (3, verts[0], 0, mf);
		if (mf->numPoints)
		{
			cm_numMarkFragments++;
			if (cm_numMarkPoints >= cm_maxMarkPoints || cm_numMarkFragments >= cm_maxMarkFragments)
				return;
		}
	}
}

/*
=================
R_RecursiveMarkFragments_r
=================
*/
static void R_RecursiveMarkFragments_r (vec3_t origin, vec3_t normal, float radius, mnode_t *node)
{
	int			i;
	float		dist;
	cplane_t	*plane;
	msurface_t	*surf;

	if (cm_numMarkPoints >= cm_maxMarkPoints || cm_numMarkFragments >= cm_maxMarkFragments)
		return; // already reached the limit somewhere else

	if (node->contents != -1)
		return;

	// find which side of the node we are on
	plane = node->plane;
	if (plane->type < 3)
		dist = origin[plane->type] - plane->dist;
	else
		dist = DotProduct(origin, plane->normal) - plane->dist;
	
	// go down the appropriate sides
	if (dist > radius)
	{
		R_RecursiveMarkFragments_r (origin, normal, radius, node->children[0]);
		return;
	}
	if (dist < -radius)
	{
		R_RecursiveMarkFragments_r (origin, normal, radius, node->children[1]);
		return;
	}
	
	// clip against each surface
	surf = r_worldmodel->surfaces + node->firstsurface;
	for (i = 0 ; i < node->numsurfaces; i++, surf++)
	{
		if (surf->fragmentframe == cm_fragmentFrame)
			continue; // already checked this surface in another node

		// bogus face
		if (surf->numvertexes < 3)
			continue;

		// greater than 60 degrees
		if (surf->flags & SURF_PLANEBACK)
		{
			if (-DotProduct(normal, surf->plane->normal) < 0.5)
				continue;
		}
		else
		{
			if (DotProduct(normal, surf->plane->normal) < 0.5)
				continue;
		}

		// not a good decal surface
		if (surf->texinfo->flags & (SURF_SKY | SURF_WARP | SURF_FLOWING | SURF_NODRAW))
			continue;

		surf->fragmentframe = cm_fragmentFrame;

		R_ClipFragmentToSurface(surf, normal, node);
	}
	
	// recurse down the children
	R_RecursiveMarkFragments_r (origin, normal, radius, node->children[0]);
	R_RecursiveMarkFragments_r (origin, normal, radius, node->children[1]);
}


/*
=================
R_GetClippedFragments
=================
*/
int R_GetClippedFragments (vec3_t origin, vec3_t axis[3], float radius, int maxPoints, vec3_t *points, int maxFragments, markFragment_t *fragments)
{
	int		i;
	float	dot;

	if (!r_worldmodel || !r_worldmodel->nodes)
		return 0; // map not loaded

	cm_fragmentFrame++; // for multi-check avoidance

	// initialize fragments
	cm_numMarkPoints = 0;
	cm_maxMarkPoints = maxPoints;
	cm_markPoints = points;

	cm_numMarkFragments = 0;
	cm_maxMarkFragments = maxFragments;
	cm_markFragments = fragments;

	// calculate clipping planes
	for (i = 0; i < 3; i++)
	{
		dot = DotProduct(origin, axis[i]);

		VectorCopy (axis[i], cm_markPlanes[i * 2 + 0].normal);
		cm_markPlanes[i * 2 + 0].dist = dot - radius;
		cm_markPlanes[i * 2 + 0].type = PlaneTypeForNormal(cm_markPlanes[i * 2 + 0].normal);

		VectorNegate (axis[i], cm_markPlanes[i * 2 + 1].normal);
		cm_markPlanes[i * 2 + 1].dist = -dot - radius;
		cm_markPlanes[i * 2 + 1].type = PlaneTypeForNormal(cm_markPlanes[i * 2 + 1].normal);
	}

	// clip against world geometry
	R_RecursiveMarkFragments_r (origin, axis[0], radius, r_worldmodel->nodes);

	return cm_numMarkFragments;
}
