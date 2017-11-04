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

// vid_public.h -- video driver defs
#ifndef _VID_PUBLIC_H_
#define _VID_PUBLIC_H_

typedef struct vidmode_s
{
	char description[64];
	int width, height;
	int mode;
} vidmode_t;

typedef struct
{
	unsigned		width, height;		// coordinates from main game
} viddef_t;
extern	viddef_t	viddef;				// global video state

// Video module initialisation etc
void	VID_Error(int err_level, char *fmt, ...);
void	VID_Printf(int print_level, char *fmt, ...);

void	VID_Init(void);
void	VID_Shutdown(void);

void	VID_WriteScreenshot(int width, int height, int comp, const void* data);

void	VID_CheckChanges(void);
qboolean VID_GetModeInfo(int *width, int *height, int mode);
void	VID_NewWindow(int width, int height);

void	VID_BeginFrame(float camera_separation);
void	VID_EndFrame(void);

qboolean VID_InitWindow(int fullscreen, int *pwidth, int *pheight);

// OpenGL specific functions
qboolean VID_Init_GL(void);
void VID_Shutdown_GL(qboolean contextOnly);

// Video menu
void	VID_MenuInit(void);
void	VID_MenuDraw(void);
const char *VID_MenuKey(int);

#endif