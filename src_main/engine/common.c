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
// common.c -- misc functions used in client and server
#include "qcommon.h"
#include <setjmp.h>
#ifdef _WIN32
#include <windows.h>
#endif

void *Scratch_Alloc (void)
{
	// this buffer should never be accessed directly but always via a call to here
	static char scratchbuf[20 * 1024 * 1024];
	return scratchbuf;
}

int		com_argc;
char	*com_argv[MAX_NUM_ARGVS+1];

int		realtime;

jmp_buf abortframe;		// an ERR_DROP occured, exit the entire frame


FILE	*log_stats_file;

cvar_t	*host_speeds;
cvar_t	*log_stats;
cvar_t	*developer;
cvar_t	*timescale;
cvar_t	*fixedtime;
cvar_t	*logfile_active;	// 1 = buffer log, 2 = flush after each print
cvar_t	*showtrace;
cvar_t	*dedicated;

// for timing calculations
cvar_t	*cl_timedemo;
cvar_t	*cl_async;
cvar_t	*cl_maxfps;
cvar_t	*r_maxfps;

FILE	*logfile;

int		server_state;

// host_speeds times
int		time_before_game;
int		time_after_game;
int		time_before_ref;
int		time_after_ref;

// used in the network and input pathes
int		curtime;

// HACK!
#ifndef DEDICATED_ONLY
int GFX_Core_GetRefreshRate(void);
qboolean VID_IsVSyncActive(void);
#endif

float r_avertexnormals[NUMVERTEXNORMALS][3] =
{
	{ -0.525731, 0.000000, 0.850651 },{ -0.442863, 0.238856, 0.864188 },{ -0.295242, 0.000000, 0.955423 },{ -0.309017, 0.500000, 0.809017 },{ -0.162460, 0.262866, 0.951056 },
	{ 0.000000, 0.000000, 1.000000 },{ 0.000000, 0.850651, 0.525731 },{ -0.147621, 0.716567, 0.681718 },{ 0.147621, 0.716567, 0.681718 },{ 0.000000, 0.525731, 0.850651 },
	{ 0.309017, 0.500000, 0.809017 },{ 0.525731, 0.000000, 0.850651 },{ 0.295242, 0.000000, 0.955423 },{ 0.442863, 0.238856, 0.864188 },{ 0.162460, 0.262866, 0.951056 },
	{ -0.681718, 0.147621, 0.716567 },{ -0.809017, 0.309017, 0.500000 },{ -0.587785, 0.425325, 0.688191 },{ -0.850651, 0.525731, 0.000000 },{ -0.864188, 0.442863, 0.238856 },
	{ -0.716567, 0.681718, 0.147621 },{ -0.688191, 0.587785, 0.425325 },{ -0.500000, 0.809017, 0.309017 },{ -0.238856, 0.864188, 0.442863 },{ -0.425325, 0.688191, 0.587785 },
	{ -0.716567, 0.681718, -0.147621 },{ -0.500000, 0.809017, -0.309017 },{ -0.525731, 0.850651, 0.000000 },{ 0.000000, 0.850651, -0.525731 },{ -0.238856, 0.864188, -0.442863 },
	{ 0.000000, 0.955423, -0.295242 },{ -0.262866, 0.951056, -0.162460 },{ 0.000000, 1.000000, 0.000000 },{ 0.000000, 0.955423, 0.295242 },{ -0.262866, 0.951056, 0.162460 },
	{ 0.238856, 0.864188, 0.442863 },{ 0.262866, 0.951056, 0.162460 },{ 0.500000, 0.809017, 0.309017 },{ 0.238856, 0.864188, -0.442863 },{ 0.262866, 0.951056, -0.162460 },
	{ 0.500000, 0.809017, -0.309017 },{ 0.850651, 0.525731, 0.000000 },{ 0.716567, 0.681718, 0.147621 },{ 0.716567, 0.681718, -0.147621 },{ 0.525731, 0.850651, 0.000000 },
	{ 0.425325, 0.688191, 0.587785 },{ 0.864188, 0.442863, 0.238856 },{ 0.688191, 0.587785, 0.425325 },{ 0.809017, 0.309017, 0.500000 },{ 0.681718, 0.147621, 0.716567 },
	{ 0.587785, 0.425325, 0.688191 },{ 0.955423, 0.295242, 0.000000 },{ 1.000000, 0.000000, 0.000000 },{ 0.951056, 0.162460, 0.262866 },{ 0.850651, -0.525731, 0.000000 },
	{ 0.955423, -0.295242, 0.000000 },{ 0.864188, -0.442863, 0.238856 },{ 0.951056, -0.162460, 0.262866 },{ 0.809017, -0.309017, 0.500000 },{ 0.681718, -0.147621, 0.716567 },
	{ 0.850651, 0.000000, 0.525731 },{ 0.864188, 0.442863, -0.238856 },{ 0.809017, 0.309017, -0.500000 },{ 0.951056, 0.162460, -0.262866 },{ 0.525731, 0.000000, -0.850651 },
	{ 0.681718, 0.147621, -0.716567 },{ 0.681718, -0.147621, -0.716567 },{ 0.850651, 0.000000, -0.525731 },{ 0.809017, -0.309017, -0.500000 },{ 0.864188, -0.442863, -0.238856 },
	{ 0.951056, -0.162460, -0.262866 },{ 0.147621, 0.716567, -0.681718 },{ 0.309017, 0.500000, -0.809017 },{ 0.425325, 0.688191, -0.587785 },{ 0.442863, 0.238856, -0.864188 },
	{ 0.587785, 0.425325, -0.688191 },{ 0.688191, 0.587785, -0.425325 },{ -0.147621, 0.716567, -0.681718 },{ -0.309017, 0.500000, -0.809017 },{ 0.000000, 0.525731, -0.850651 },
	{ -0.525731, 0.000000, -0.850651 },{ -0.442863, 0.238856, -0.864188 },{ -0.295242, 0.000000, -0.955423 },{ -0.162460, 0.262866, -0.951056 },{ 0.000000, 0.000000, -1.000000 },
	{ 0.295242, 0.000000, -0.955423 },{ 0.162460, 0.262866, -0.951056 },{ -0.442863, -0.238856, -0.864188 },{ -0.309017, -0.500000, -0.809017 },{ -0.162460, -0.262866, -0.951056 },
	{ 0.000000, -0.850651, -0.525731 },{ -0.147621, -0.716567, -0.681718 },{ 0.147621, -0.716567, -0.681718 },{ 0.000000, -0.525731, -0.850651 },{ 0.309017, -0.500000, -0.809017 },
	{ 0.442863, -0.238856, -0.864188 },{ 0.162460, -0.262866, -0.951056 },{ 0.238856, -0.864188, -0.442863 },{ 0.500000, -0.809017, -0.309017 },{ 0.425325, -0.688191, -0.587785 },
	{ 0.716567, -0.681718, -0.147621 },{ 0.688191, -0.587785, -0.425325 },{ 0.587785, -0.425325, -0.688191 },{ 0.000000, -0.955423, -0.295242 },{ 0.000000, -1.000000, 0.000000 },
	{ 0.262866, -0.951056, -0.162460 },{ 0.000000, -0.850651, 0.525731 },{ 0.000000, -0.955423, 0.295242 },{ 0.238856, -0.864188, 0.442863 },{ 0.262866, -0.951056, 0.162460 },
	{ 0.500000, -0.809017, 0.309017 },{ 0.716567, -0.681718, 0.147621 },{ 0.525731, -0.850651, 0.000000 },{ -0.238856, -0.864188, -0.442863 },{ -0.500000, -0.809017, -0.309017 },
	{ -0.262866, -0.951056, -0.162460 },{ -0.850651, -0.525731, 0.000000 },{ -0.716567, -0.681718, -0.147621 },{ -0.716567, -0.681718, 0.147621 },{ -0.525731, -0.850651, 0.000000 },
	{ -0.500000, -0.809017, 0.309017 },{ -0.238856, -0.864188, 0.442863 },{ -0.262866, -0.951056, 0.162460 },{ -0.864188, -0.442863, 0.238856 },{ -0.809017, -0.309017, 0.500000 },
	{ -0.688191, -0.587785, 0.425325 },{ -0.681718, -0.147621, 0.716567 },{ -0.442863, -0.238856, 0.864188 },{ -0.587785, -0.425325, 0.688191 },{ -0.309017, -0.500000, 0.809017 },
	{ -0.147621, -0.716567, 0.681718 },{ -0.425325, -0.688191, 0.587785 },{ -0.162460, -0.262866, 0.951056 },{ 0.442863, -0.238856, 0.864188 },{ 0.162460, -0.262866, 0.951056 },
	{ 0.309017, -0.500000, 0.809017 },{ 0.147621, -0.716567, 0.681718 },{ 0.000000, -0.525731, 0.850651 },{ 0.425325, -0.688191, 0.587785 },{ 0.587785, -0.425325, 0.688191 },
	{ 0.688191, -0.587785, 0.425325 },{ -0.955423, 0.295242, 0.000000 },{ -0.951056, 0.162460, 0.262866 },{ -1.000000, 0.000000, 0.000000 },{ -0.850651, 0.000000, 0.525731 },
	{ -0.955423, -0.295242, 0.000000 },{ -0.951056, -0.162460, 0.262866 },{ -0.864188, 0.442863, -0.238856 },{ -0.951056, 0.162460, -0.262866 },{ -0.809017, 0.309017, -0.500000 },
	{ -0.864188, -0.442863, -0.238856 },{ -0.951056, -0.162460, -0.262866 },{ -0.809017, -0.309017, -0.500000 },{ -0.681718, 0.147621, -0.716567 },{ -0.681718, -0.147621, -0.716567 },
	{ -0.850651, 0.000000, -0.525731 },{ -0.688191, 0.587785, -0.425325 },{ -0.587785, 0.425325, -0.688191 },{ -0.425325, 0.688191, -0.587785 },{ -0.425325, -0.688191, -0.587785 },
	{ -0.587785, -0.425325, -0.688191 },{ -0.688191, -0.587785, -0.425325 }
};

/*
============================================================================

CLIENT / SERVER interactions

============================================================================
*/

static int	rd_target;
static char	*rd_buffer;
static int	rd_buffersize;
static void	(*rd_flush) (int target, char *buffer);

void Com_BeginRedirect (int target, char *buffer, int buffersize, void (*flush))
{
	if (!target || !buffer || !buffersize || !flush)
		return;

	rd_target = target;
	rd_buffer = buffer;
	rd_buffersize = buffersize;
	rd_flush = flush;

	*rd_buffer = 0;
}

void Com_EndRedirect (void)
{
	rd_flush (rd_target, rd_buffer);

	rd_target = 0;
	rd_buffer = NULL;
	rd_buffersize = 0;
	rd_flush = NULL;
}

/*
=============
Com_Printf

Both client and server can use this, and it will output
to the apropriate place.
=============
*/
void Com_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start (argptr, fmt);
	vsprintf (msg, fmt, argptr);
	va_end (argptr);

	if (rd_target)
	{
		if ((strlen (msg) + strlen (rd_buffer)) > (rd_buffersize - 1))
		{
			rd_flush (rd_target, rd_buffer);
			*rd_buffer = 0;
		}

		strcat (rd_buffer, msg);
		return;
	}

	// print to ingame console
	Con_Print (msg);

	// also echo to console
	Sys_Print (msg);

	// logfile
	if (logfile_active && logfile_active->value)
	{
		char	name[MAX_QPATH];

		if (!logfile)
		{
			Com_sprintf (name, sizeof (name), "%s/qconsole.log", FS_Gamedir ());

			if (logfile_active->value > 2)
				logfile = fopen (name, "a");
			else
				logfile = fopen (name, "w");
		}

		if (logfile)
			fprintf (logfile, "%s", msg);

		if (logfile_active->value > 1)
			fflush (logfile);		// force it to save every time
	}
}


/*
================
Com_DPrintf

A Com_Printf that only shows up if the "developer" cvar is set
================
*/
void Com_DPrintf (char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	if (!developer || !developer->value)
		return;			// don't confuse non-developers with techie stuff...

	va_start (argptr, fmt);
	vsprintf (msg, fmt, argptr);
	va_end (argptr);

	Com_Printf ("%s", msg);
}


/*
=============
Com_Error

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Com_Error (int code, char *fmt, ...)
{
	va_list		argptr;
	static char		msg[MAXPRINTMSG];
	static	qboolean	recursive;

	if (recursive)
		Sys_Error (S_COLOR_RED "recursive error after: %s" S_COLOR_WHITE, msg);

	recursive = true;

	va_start (argptr, fmt);
	vsprintf (msg, fmt, argptr);
	va_end (argptr);

	if (code == ERR_DISCONNECT)
	{
		CL_Drop ();
		recursive = false;
		longjmp (abortframe, -1);
	}
	else if (code == ERR_DROP)
	{
		Com_Printf (S_COLOR_RED "********************\n" S_COLOR_RED "ERROR: %s\n" S_COLOR_RED "********************\n", msg);
		SV_Shutdown (va (S_COLOR_RED "Server crashed: %s\n" S_COLOR_WHITE, msg), false);
		CL_Drop ();
		recursive = false;
		longjmp (abortframe, -1);
	}
	else
	{
		SV_Shutdown (va (S_COLOR_RED "Server fatal crashed: %s\n" S_COLOR_WHITE, msg), false);
		CL_Shutdown ();
	}

	if (logfile)
	{
		fclose (logfile);
		logfile = NULL;
	}

	Sys_Error ("%s", msg);
}


/*
=============
Com_Quit

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Com_Quit (void)
{
	SV_Shutdown ("Server quit\n", false);
	CL_Shutdown ();

	if (logfile)
	{
		fclose (logfile);
		logfile = NULL;
	}

	Sys_Shutdown ();
}


/*
==================
Com_ServerState
==================
*/
int Com_ServerState (void)
{
	return server_state;
}

/*
==================
Com_SetServerState
==================
*/
void Com_SetServerState (int state)
{
	server_state = state;
}


/*
==============================================================================

			MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/


//
// writing functions
//

void MSG_WriteChar (sizebuf_t *sb, int c)
{
	byte	*buf;

#ifdef PARANOID

	if (c < -128 || c > 127)
		Com_Error (ERR_FATAL, "MSG_WriteChar: range error");

#endif

	buf = SZ_GetSpace (sb, 1);
	buf[0] = c;
}

void MSG_WriteByte (sizebuf_t *sb, int c)
{
	byte	*buf;

#ifdef PARANOID

	if (c < 0 || c > 255)
		Com_Error (ERR_FATAL, "MSG_WriteByte: range error");

#endif

	buf = SZ_GetSpace (sb, 1);
	buf[0] = c;
}

void MSG_WriteShort (sizebuf_t *sb, int c)
{
	byte	*buf;

#ifdef PARANOID

	if (c < ((short) 0x8000) || c > (short) 0x7fff)
		Com_Error (ERR_FATAL, "MSG_WriteShort: range error");

#endif

	buf = SZ_GetSpace (sb, 2);
	buf[0] = c & 0xff;
	buf[1] = c >> 8;
}

void MSG_WriteLong (sizebuf_t *sb, int c)
{
	byte	*buf;

	buf = SZ_GetSpace (sb, 4);
	buf[0] = c & 0xff;
	buf[1] = (c >> 8) & 0xff;
	buf[2] = (c >> 16) & 0xff;
	buf[3] = c >> 24;
}

void MSG_WriteFloat (sizebuf_t *sb, float f)
{
	union
	{
		float	f;
		int	l;
	} dat;


	dat.f = f;
	dat.l = LittleLong (dat.l);

	SZ_Write (sb, &dat.l, 4);
}

void MSG_WriteString (sizebuf_t *sb, char *s)
{
	if (!s)
		SZ_Write (sb, "", 1);
	else
		SZ_Write (sb, s, strlen (s) + 1);
}

void MSG_WriteCoord (sizebuf_t *sb, float f)
{
	MSG_WriteShort (sb, (int) (f * 8));
}

void MSG_WritePos (sizebuf_t *sb, vec3_t pos)
{
	MSG_WriteShort (sb, (int) (pos[0] * 8));
	MSG_WriteShort (sb, (int) (pos[1] * 8));
	MSG_WriteShort (sb, (int) (pos[2] * 8));
}

void MSG_WriteAngle (sizebuf_t *sb, float f)
{
	MSG_WriteByte (sb, (int) (f * 256 / 360) & 255);
}

void MSG_WriteAngle16 (sizebuf_t *sb, float f)
{
	MSG_WriteShort (sb, ANGLE2SHORT (f));
}


void MSG_WriteDeltaUsercmd (sizebuf_t *buf, usercmd_t *from, usercmd_t *cmd)
{
	int		bits;

	//
	// send the movement message
	//
	bits = 0;

	if (cmd->angles[0] != from->angles[0])
		bits |= CM_ANGLE1;

	if (cmd->angles[1] != from->angles[1])
		bits |= CM_ANGLE2;

	if (cmd->angles[2] != from->angles[2])
		bits |= CM_ANGLE3;

	if (cmd->forwardmove != from->forwardmove)
		bits |= CM_FORWARD;

	if (cmd->sidemove != from->sidemove)
		bits |= CM_SIDE;

	if (cmd->upmove != from->upmove)
		bits |= CM_UP;

	if (cmd->buttons != from->buttons)
		bits |= CM_BUTTONS;

	if (cmd->impulse != from->impulse)
		bits |= CM_IMPULSE;

	MSG_WriteByte (buf, bits);

	if (bits & CM_ANGLE1)
		MSG_WriteShort (buf, cmd->angles[0]);

	if (bits & CM_ANGLE2)
		MSG_WriteShort (buf, cmd->angles[1]);

	if (bits & CM_ANGLE3)
		MSG_WriteShort (buf, cmd->angles[2]);

	if (bits & CM_FORWARD)
		MSG_WriteShort (buf, cmd->forwardmove);

	if (bits & CM_SIDE)
		MSG_WriteShort (buf, cmd->sidemove);

	if (bits & CM_UP)
		MSG_WriteShort (buf, cmd->upmove);

	if (bits & CM_BUTTONS)
		MSG_WriteByte (buf, cmd->buttons);

	if (bits & CM_IMPULSE)
		MSG_WriteByte (buf, cmd->impulse);

	MSG_WriteByte (buf, cmd->msec);
	MSG_WriteByte (buf, cmd->lightlevel);
}


void MSG_WriteDir (sizebuf_t *sb, vec3_t dir)
{
	int		i, best;
	float	d, bestd;

	if (!dir)
	{
		MSG_WriteByte (sb, 0);
		return;
	}

	bestd = 0;
	best = 0;

	for (i = 0; i < NUMVERTEXNORMALS; i++)
	{
		d = DotProduct (dir, r_avertexnormals[i]);

		if (d > bestd)
		{
			bestd = d;
			best = i;
		}
	}

	MSG_WriteByte (sb, best);
}


void MSG_ReadDir (sizebuf_t *sb, vec3_t dir)
{
	int		b;

	b = MSG_ReadByte (sb);

	if (b >= NUMVERTEXNORMALS)
		Com_Error (ERR_DROP, "MSF_ReadDir: out of range");

	VectorCopy (r_avertexnormals[b], dir);
}


/*
==================
MSG_WriteDeltaEntity

Writes part of a packetentities message.
Can delta from either a baseline or a previous packet_entity
==================
*/
void MSG_WriteDeltaEntity (entity_state_t *from, entity_state_t *to, sizebuf_t *msg, qboolean force, qboolean newentity)
{
	int		bits;

	if (!to->number)
		Com_Error (ERR_FATAL, "Unset entity number");

	if (to->number >= MAX_EDICTS)
		Com_Error (ERR_FATAL, "Entity number >= MAX_EDICTS");

	// send an update
	bits = 0;

	if (to->number >= 256)
		bits |= U_NUMBER16;		// number8 is implicit otherwise

	if (to->origin[0] != from->origin[0])
		bits |= U_ORIGIN1;

	if (to->origin[1] != from->origin[1])
		bits |= U_ORIGIN2;

	if (to->origin[2] != from->origin[2])
		bits |= U_ORIGIN3;

	if (to->angles[0] != from->angles[0])
		bits |= U_ANGLE1;

	if (to->angles[1] != from->angles[1])
		bits |= U_ANGLE2;

	if (to->angles[2] != from->angles[2])
		bits |= U_ANGLE3;

	if (to->skinnum != from->skinnum)
	{
		if ((unsigned) to->skinnum < 256)
			bits |= U_SKIN8;
		else if ((unsigned) to->skinnum < 0x10000)
			bits |= U_SKIN16;
		else
			bits |= (U_SKIN8 | U_SKIN16);
	}

	if (to->frame != from->frame)
	{
		if (to->frame < 256)
			bits |= U_FRAME8;
		else
			bits |= U_FRAME16;
	}

	if (to->effects != from->effects)
	{
		if (to->effects < 256)
			bits |= U_EFFECTS8;
		else if (to->effects < 0x8000)
			bits |= U_EFFECTS16;
		else
			bits |= U_EFFECTS8 | U_EFFECTS16;
	}

	if (to->renderfx != from->renderfx)
	{
		if (to->renderfx < 256)
			bits |= U_RENDERFX8;
		else if (to->renderfx < 0x8000)
			bits |= U_RENDERFX16;
		else
			bits |= U_RENDERFX8 | U_RENDERFX16;
	}

	if (to->solid != from->solid)
		bits |= U_SOLID;

	// event is not delta compressed, just 0 compressed
	if (to->event)
		bits |= U_EVENT;

	if (to->modelindex != from->modelindex)
		bits |= U_MODEL;

	if (to->modelindex2 != from->modelindex2)
		bits |= U_MODEL2;

	if (to->modelindex3 != from->modelindex3)
		bits |= U_MODEL3;

	if (to->modelindex4 != from->modelindex4)
		bits |= U_MODEL4;

	if (to->sound != from->sound)
		bits |= U_SOUND;

	if (newentity || (to->renderfx & RF_BEAM))
		bits |= U_OLDORIGIN;

	//
	// write the message
	//
	if (!bits && !force)
		return;		// nothing to send!

	//----------

	if (bits & 0xff000000)
		bits |= U_MOREBITS3 | U_MOREBITS2 | U_MOREBITS1;
	else if (bits & 0x00ff0000)
		bits |= U_MOREBITS2 | U_MOREBITS1;
	else if (bits & 0x0000ff00)
		bits |= U_MOREBITS1;

	MSG_WriteByte (msg,	bits & 255);

	if (bits & 0xff000000)
	{
		MSG_WriteByte (msg,	(bits >> 8) & 255);
		MSG_WriteByte (msg,	(bits >> 16) & 255);
		MSG_WriteByte (msg,	(bits >> 24) & 255);
	}
	else if (bits & 0x00ff0000)
	{
		MSG_WriteByte (msg,	(bits >> 8) & 255);
		MSG_WriteByte (msg,	(bits >> 16) & 255);
	}
	else if (bits & 0x0000ff00)
	{
		MSG_WriteByte (msg,	(bits >> 8) & 255);
	}

	//----------

	if (bits & U_NUMBER16)
		MSG_WriteShort (msg, to->number);
	else
		MSG_WriteByte (msg,	to->number);

	if (bits & U_MODEL)
		MSG_WriteByte (msg,	to->modelindex);

	if (bits & U_MODEL2)
		MSG_WriteByte (msg,	to->modelindex2);

	if (bits & U_MODEL3)
		MSG_WriteByte (msg,	to->modelindex3);

	if (bits & U_MODEL4)
		MSG_WriteByte (msg,	to->modelindex4);

	if (bits & U_FRAME8)
		MSG_WriteByte (msg, to->frame);

	if (bits & U_FRAME16)
		MSG_WriteShort (msg, to->frame);

	if ((bits & U_SKIN8) && (bits & U_SKIN16))		//used for laser colors
		MSG_WriteLong (msg, to->skinnum);
	else if (bits & U_SKIN8)
		MSG_WriteByte (msg, to->skinnum);
	else if (bits & U_SKIN16)
		MSG_WriteShort (msg, to->skinnum);


	if ((bits & (U_EFFECTS8 | U_EFFECTS16)) == (U_EFFECTS8 | U_EFFECTS16))
		MSG_WriteLong (msg, to->effects);
	else if (bits & U_EFFECTS8)
		MSG_WriteByte (msg, to->effects);
	else if (bits & U_EFFECTS16)
		MSG_WriteShort (msg, to->effects);

	if ((bits & (U_RENDERFX8 | U_RENDERFX16)) == (U_RENDERFX8 | U_RENDERFX16))
		MSG_WriteLong (msg, to->renderfx);
	else if (bits & U_RENDERFX8)
		MSG_WriteByte (msg, to->renderfx);
	else if (bits & U_RENDERFX16)
		MSG_WriteShort (msg, to->renderfx);

	if (bits & U_ORIGIN1)
		MSG_WriteCoord (msg, to->origin[0]);

	if (bits & U_ORIGIN2)
		MSG_WriteCoord (msg, to->origin[1]);

	if (bits & U_ORIGIN3)
		MSG_WriteCoord (msg, to->origin[2]);

	if (bits & U_ANGLE1)
		MSG_WriteAngle (msg, to->angles[0]);

	if (bits & U_ANGLE2)
		MSG_WriteAngle (msg, to->angles[1]);

	if (bits & U_ANGLE3)
		MSG_WriteAngle (msg, to->angles[2]);

	if (bits & U_OLDORIGIN)
	{
		MSG_WriteCoord (msg, to->old_origin[0]);
		MSG_WriteCoord (msg, to->old_origin[1]);
		MSG_WriteCoord (msg, to->old_origin[2]);
	}

	if (bits & U_SOUND)
		MSG_WriteByte (msg, to->sound);

	if (bits & U_EVENT)
		MSG_WriteByte (msg, to->event);

	if (bits & U_SOLID)
		MSG_WriteShort (msg, to->solid);
}


//============================================================

//
// reading functions
//

void MSG_BeginReading (sizebuf_t *msg)
{
	msg->readcount = 0;
}

// returns -1 if no more characters are available
int MSG_ReadChar (sizebuf_t *msg_read)
{
	int	c;

	if (msg_read->readcount + 1 > msg_read->cursize)
		c = -1;
	else
		c = (signed char) msg_read->data[msg_read->readcount];

	msg_read->readcount++;

	return c;
}

int MSG_ReadByte (sizebuf_t *msg_read)
{
	int	c;

	if (msg_read->readcount + 1 > msg_read->cursize)
		c = -1;
	else
		c = (unsigned char) msg_read->data[msg_read->readcount];

	msg_read->readcount++;

	return c;
}

int MSG_ReadShort (sizebuf_t *msg_read)
{
	int	c;

	if (msg_read->readcount + 2 > msg_read->cursize)
		c = -1;
	else
		c = (short) (msg_read->data[msg_read->readcount]
					 + (msg_read->data[msg_read->readcount+1] << 8));

	msg_read->readcount += 2;

	return c;
}

int MSG_ReadLong (sizebuf_t *msg_read)
{
	int	c;

	if (msg_read->readcount + 4 > msg_read->cursize)
		c = -1;
	else
		c = msg_read->data[msg_read->readcount]
			+ (msg_read->data[msg_read->readcount+1] << 8)
			+ (msg_read->data[msg_read->readcount+2] << 16)
			+ (msg_read->data[msg_read->readcount+3] << 24);

	msg_read->readcount += 4;

	return c;
}

float MSG_ReadFloat (sizebuf_t *msg_read)
{
	union
	{
		byte	b[4];
		float	f;
		int	l;
	} dat;

	if (msg_read->readcount + 4 > msg_read->cursize)
		dat.f = -1;
	else
	{
		dat.b[0] =	msg_read->data[msg_read->readcount];
		dat.b[1] =	msg_read->data[msg_read->readcount+1];
		dat.b[2] =	msg_read->data[msg_read->readcount+2];
		dat.b[3] =	msg_read->data[msg_read->readcount+3];
	}

	msg_read->readcount += 4;

	dat.l = LittleLong (dat.l);

	return dat.f;
}

char *MSG_ReadString (sizebuf_t *msg_read)
{
	static char	string[2048];
	int		l, c;

	l = 0;

	do
	{
		c = MSG_ReadChar (msg_read);

		if (c == -1 || c == 0)
			break;

		string[l] = c;
		l++;
	}
	while (l < sizeof (string) - 1);

	string[l] = 0;

	return string;
}

char *MSG_ReadStringLine (sizebuf_t *msg_read)
{
	static char	string[2048];
	int		l, c;

	l = 0;

	do
	{
		c = MSG_ReadChar (msg_read);

		if (c == -1 || c == 0 || c == '\n')
			break;

		string[l] = c;
		l++;
	}
	while (l < sizeof (string) - 1);

	string[l] = 0;

	return string;
}

float MSG_ReadCoord (sizebuf_t *msg_read)
{
	return MSG_ReadShort (msg_read) * (1.0 / 8);
}

void MSG_ReadPos (sizebuf_t *msg_read, vec3_t pos)
{
	pos[0] = MSG_ReadShort (msg_read) * (1.0 / 8);
	pos[1] = MSG_ReadShort (msg_read) * (1.0 / 8);
	pos[2] = MSG_ReadShort (msg_read) * (1.0 / 8);
}

float MSG_ReadAngle (sizebuf_t *msg_read)
{
	return MSG_ReadChar (msg_read) * (360.0 / 256);
}

float MSG_ReadAngle16 (sizebuf_t *msg_read)
{
	return SHORT2ANGLE (MSG_ReadShort (msg_read));
}

void MSG_ReadDeltaUsercmd (sizebuf_t *msg_read, usercmd_t *from, usercmd_t *move)
{
	int bits;

	memcpy (move, from, sizeof (*move));

	bits = MSG_ReadByte (msg_read);

	// read current angles
	if (bits & CM_ANGLE1)
		move->angles[0] = MSG_ReadShort (msg_read);

	if (bits & CM_ANGLE2)
		move->angles[1] = MSG_ReadShort (msg_read);

	if (bits & CM_ANGLE3)
		move->angles[2] = MSG_ReadShort (msg_read);

	// read movement
	if (bits & CM_FORWARD)
		move->forwardmove = MSG_ReadShort (msg_read);

	if (bits & CM_SIDE)
		move->sidemove = MSG_ReadShort (msg_read);

	if (bits & CM_UP)
		move->upmove = MSG_ReadShort (msg_read);

	// read buttons
	if (bits & CM_BUTTONS)
		move->buttons = MSG_ReadByte (msg_read);

	if (bits & CM_IMPULSE)
		move->impulse = MSG_ReadByte (msg_read);

	// read time to run command
	move->msec = MSG_ReadByte (msg_read);

	// read the light level
	move->lightlevel = MSG_ReadByte (msg_read);
}


void MSG_ReadData (sizebuf_t *msg_read, void *data, int len)
{
	int		i;

	for (i = 0; i < len; i++)
		((byte *) data) [i] = MSG_ReadByte (msg_read);
}


//===========================================================================

void SZ_Init (sizebuf_t *buf, byte *data, int length)
{
	memset (buf, 0, sizeof (*buf));
	buf->data = data;
	buf->maxsize = length;
}

void SZ_Clear (sizebuf_t *buf)
{
	buf->cursize = 0;
	buf->overflowed = false;
}

void *SZ_GetSpace (sizebuf_t *buf, int length)
{
	void	*data;

	if (buf->cursize + length > buf->maxsize)
	{
		if (!buf->allowoverflow)
			Com_Error (ERR_FATAL, "SZ_GetSpace: overflow without allowoverflow set");

		if (length > buf->maxsize)
			Com_Error (ERR_FATAL, "SZ_GetSpace: %i is > full buffer size", length);

		Com_Printf (S_COLOR_RED "SZ_GetSpace: overflow\n");
		SZ_Clear (buf);
		buf->overflowed = true;
	}

	data = buf->data + buf->cursize;
	buf->cursize += length;

	return data;
}

void SZ_Write (sizebuf_t *buf, void *data, int length)
{
	memcpy (SZ_GetSpace (buf, length), data, length);
}

void SZ_Print (sizebuf_t *buf, char *data)
{
	int		len;

	len = strlen (data) + 1;

	if (buf->cursize)
	{
		if (buf->data[buf->cursize-1])
			memcpy ((byte *) SZ_GetSpace (buf, len), data, len); // no trailing 0
		else
			memcpy ((byte *) SZ_GetSpace (buf, len - 1) - 1, data, len); // write over trailing 0
	}
	else
		memcpy ((byte *) SZ_GetSpace (buf, len), data, len);
}


//============================================================================


/*
================
COM_CheckParm

Returns the position (1 to argc-1) in the program's argument list
where the given parameter apears, or 0 if not present
================
*/
int COM_CheckParm (char *parm)
{
	int		i;

	for (i = 1; i < com_argc; i++)
	{
		if (!strcmp (parm, com_argv[i]))
			return i;
	}

	return 0;
}

int COM_Argc (void)
{
	return com_argc;
}

char *COM_Argv (int arg)
{
	if (arg < 0 || arg >= com_argc || !com_argv[arg])
		return "";

	return com_argv[arg];
}

void COM_ClearArgv (int arg)
{
	if (arg < 0 || arg >= com_argc || !com_argv[arg])
		return;

	com_argv[arg] = "";
}


/*
================
COM_InitArgv
================
*/
void COM_InitArgv (int argc, char **argv)
{
	int		i;

	if (argc > MAX_NUM_ARGVS)
		Com_Error (ERR_FATAL, "argc > MAX_NUM_ARGVS");

	com_argc = argc;

	for (i = 0; i < argc; i++)
	{
		if (!argv[i] || strlen (argv[i]) >= MAX_TOKEN_CHARS)
			com_argv[i] = "";
		else
			com_argv[i] = argv[i];
	}
}

/*
================
COM_AddParm

Adds the given string at the end of the current argument list
================
*/
void COM_AddParm (char *parm)
{
	if (com_argc == MAX_NUM_ARGVS)
		Com_Error (ERR_FATAL, "COM_AddParm: MAX_NUM)ARGS");

	com_argv[com_argc++] = parm;
}


char *CopyString (char *in)
{
	char	*out;

	out = Z_Malloc (strlen (in) + 1);
	strcpy (out, in);
	return out;
}

void Info_Print (char *s)
{
	char	key[512];
	char	value[512];
	char	*o;
	int		l;

	if (*s == '\\')
		s++;

	while (*s)
	{
		o = key;

		while (*s && *s != '\\')
			*o++ = *s++;

		l = o - key;

		if (l < 20)
		{
			memset (o, ' ', 20 - l);
			key[20] = 0;
		}
		else
			*o = 0;

		Com_Printf ("%s", key);

		if (!*s)
		{
			Com_Printf (S_COLOR_RED "MISSING VALUE\n");
			return;
		}

		o = value;
		s++;

		while (*s && *s != '\\')
			*o++ = *s++;

		*o = 0;

		if (*s)
			s++;

		Com_Printf ("%s\n", value);
	}
}

/*
================
Com_RealTime
================
*/
int Com_RealTime (qtime_t * qtime)
{
	time_t t;
	struct tm *tms;

	t = time (NULL);
	if (!qtime)
		return t;

	tms = localtime (&t);
	if (tms)
	{
		qtime->tm_sec = tms->tm_sec;
		qtime->tm_min = tms->tm_min;
		qtime->tm_hour = tms->tm_hour;
		qtime->tm_mday = tms->tm_mday;
		qtime->tm_mon = tms->tm_mon;
		qtime->tm_year = tms->tm_year;
		qtime->tm_wday = tms->tm_wday;
		qtime->tm_yday = tms->tm_yday;
		qtime->tm_isdst = tms->tm_isdst;
	}
	return t;
}

/*
==============================================================================

						ZONE MEMORY ALLOCATION

just cleared malloc with counters now...

==============================================================================
*/

#define	Z_MAGIC		0x1d1d

typedef struct zhead_s
{
	struct zhead_s	*prev, *next;
	short	magic;
	short	tag;			// for group free
	int		size;
} zhead_t;
static zhead_t z_chain = { 0 };

int		z_count, z_bytes;

/*
========================
Z_Free
========================
*/
void Z_Free (void *ptr)
{
	zhead_t	*z;

	z = ((zhead_t *)ptr) - 1;

	if (z->magic != Z_MAGIC)
		Com_Error (ERR_FATAL, "Z_Free: bad magic");

	z->prev->next = z->next;
	z->next->prev = z->prev;

	z_count--;
	z_bytes -= z->size;
	free (z);
}


/*
========================
Z_Stats_f
========================
*/
void Z_Stats_f (void)
{
	Com_Printf ("%i bytes in %i blocks\n", z_bytes, z_count);
}

/*
========================
Z_FreeTags
========================
*/
void Z_FreeTags (int tag)
{
	zhead_t	*z, *next;

	for (z=z_chain.next ; z != &z_chain ; z=next)
	{
		next = z->next;
		if (z->tag == tag)
			Z_Free ((void *)(z+1));
	}
}

/*
========================
Z_TagMalloc
========================
*/
void *Z_TagMalloc (int size, int tag)
{
	zhead_t	*z;

	size = size + sizeof(zhead_t);
	z = malloc(size);
	if (!z)
		Com_Error (ERR_FATAL, "Z_Malloc: failed on allocation of %i bytes",size);
	memset (z, 0, size);
	z_count++;
	z_bytes += size;
	z->magic = Z_MAGIC;
	z->tag = tag;
	z->size = size;

	z->next = z_chain.next;
	z->prev = &z_chain;
	z_chain.next->prev = z;
	z_chain.next = z;

	return (void *)(z+1);
}

/*
========================
Z_Malloc
========================
*/
void *Z_Malloc (int size)
{
	return Z_TagMalloc (size, 0);
}


//============================================================================

static byte chktbl[1024] =
{
	0x84, 0x47, 0x51, 0xc1, 0x93, 0x22, 0x21, 0x24, 0x2f, 0x66, 0x60, 0x4d, 0xb0, 0x7c, 0xda,
	0x88, 0x54, 0x15, 0x2b, 0xc6, 0x6c, 0x89, 0xc5, 0x9d, 0x48, 0xee, 0xe6, 0x8a, 0xb5, 0xf4,
	0xcb, 0xfb, 0xf1, 0x0c, 0x2e, 0xa0, 0xd7, 0xc9, 0x1f, 0xd6, 0x06, 0x9a, 0x09, 0x41, 0x54,
	0x67, 0x46, 0xc7, 0x74, 0xe3, 0xc8, 0xb6, 0x5d, 0xa6, 0x36, 0xc4, 0xab, 0x2c, 0x7e, 0x85,
	0xa8, 0xa4, 0xa6, 0x4d, 0x96, 0x19, 0x19, 0x9a, 0xcc, 0xd8, 0xac, 0x39, 0x5e, 0x3c, 0xf2,
	0xf5, 0x5a, 0x72, 0xe5, 0xa9, 0xd1, 0xb3, 0x23, 0x82, 0x6f, 0x29, 0xcb, 0xd1, 0xcc, 0x71,
	0xfb, 0xea, 0x92, 0xeb, 0x1c, 0xca, 0x4c, 0x70, 0xfe, 0x4d, 0xc9, 0x67, 0x43, 0x47, 0x94,
	0xb9, 0x47, 0xbc, 0x3f, 0x01, 0xab, 0x7b, 0xa6, 0xe2, 0x76, 0xef, 0x5a, 0x7a, 0x29, 0x0b,
	0x51, 0x54, 0x67, 0xd8, 0x1c, 0x14, 0x3e, 0x29, 0xec, 0xe9, 0x2d, 0x48, 0x67, 0xff, 0xed,
	0x54, 0x4f, 0x48, 0xc0, 0xaa, 0x61, 0xf7, 0x78, 0x12, 0x03, 0x7a, 0x9e, 0x8b, 0xcf, 0x83,
	0x7b, 0xae, 0xca, 0x7b, 0xd9, 0xe9, 0x53, 0x2a, 0xeb, 0xd2, 0xd8, 0xcd, 0xa3, 0x10, 0x25,
	0x78, 0x5a, 0xb5, 0x23, 0x06, 0x93, 0xb7, 0x84, 0xd2, 0xbd, 0x96, 0x75, 0xa5, 0x5e, 0xcf,
	0x4e, 0xe9, 0x50, 0xa1, 0xe6, 0x9d, 0xb1, 0xe3, 0x85, 0x66, 0x28, 0x4e, 0x43, 0xdc, 0x6e,
	0xbb, 0x33, 0x9e, 0xf3, 0x0d, 0x00, 0xc1, 0xcf, 0x67, 0x34, 0x06, 0x7c, 0x71, 0xe3, 0x63,
	0xb7, 0xb7, 0xdf, 0x92, 0xc4, 0xc2, 0x25, 0x5c, 0xff, 0xc3, 0x6e, 0xfc, 0xaa, 0x1e, 0x2a,
	0x48, 0x11, 0x1c, 0x36, 0x68, 0x78, 0x86, 0x79, 0x30, 0xc3, 0xd6, 0xde, 0xbc, 0x3a, 0x2a,
	0x6d, 0x1e, 0x46, 0xdd, 0xe0, 0x80, 0x1e, 0x44, 0x3b, 0x6f, 0xaf, 0x31, 0xda, 0xa2, 0xbd,
	0x77, 0x06, 0x56, 0xc0, 0xb7, 0x92, 0x4b, 0x37, 0xc0, 0xfc, 0xc2, 0xd5, 0xfb, 0xa8, 0xda,
	0xf5, 0x57, 0xa8, 0x18, 0xc0, 0xdf, 0xe7, 0xaa, 0x2a, 0xe0, 0x7c, 0x6f, 0x77, 0xb1, 0x26,
	0xba, 0xf9, 0x2e, 0x1d, 0x16, 0xcb, 0xb8, 0xa2, 0x44, 0xd5, 0x2f, 0x1a, 0x79, 0x74, 0x87,
	0x4b, 0x00, 0xc9, 0x4a, 0x3a, 0x65, 0x8f, 0xe6, 0x5d, 0xe5, 0x0a, 0x77, 0xd8, 0x1a, 0x14,
	0x41, 0x75, 0xb1, 0xe2, 0x50, 0x2c, 0x93, 0x38, 0x2b, 0x6d, 0xf3, 0xf6, 0xdb, 0x1f, 0xcd,
	0xff, 0x14, 0x70, 0xe7, 0x16, 0xe8, 0x3d, 0xf0, 0xe3, 0xbc, 0x5e, 0xb6, 0x3f, 0xcc, 0x81,
	0x24, 0x67, 0xf3, 0x97, 0x3b, 0xfe, 0x3a, 0x96, 0x85, 0xdf, 0xe4, 0x6e, 0x3c, 0x85, 0x05,
	0x0e, 0xa3, 0x2b, 0x07, 0xc8, 0xbf, 0xe5, 0x13, 0x82, 0x62, 0x08, 0x61, 0x69, 0x4b, 0x47,
	0x62, 0x73, 0x44, 0x64, 0x8e, 0xe2, 0x91, 0xa6, 0x9a, 0xb7, 0xe9, 0x04, 0xb6, 0x54, 0x0c,
	0xc5, 0xa9, 0x47, 0xa6, 0xc9, 0x08, 0xfe, 0x4e, 0xa6, 0xcc, 0x8a, 0x5b, 0x90, 0x6f, 0x2b,
	0x3f, 0xb6, 0x0a, 0x96, 0xc0, 0x78, 0x58, 0x3c, 0x76, 0x6d, 0x94, 0x1a, 0xe4, 0x4e, 0xb8,
	0x38, 0xbb, 0xf5, 0xeb, 0x29, 0xd8, 0xb0, 0xf3, 0x15, 0x1e, 0x99, 0x96, 0x3c, 0x5d, 0x63,
	0xd5, 0xb1, 0xad, 0x52, 0xb8, 0x55, 0x70, 0x75, 0x3e, 0x1a, 0xd5, 0xda, 0xf6, 0x7a, 0x48,
	0x7d, 0x44, 0x41, 0xf9, 0x11, 0xce, 0xd7, 0xca, 0xa5, 0x3d, 0x7a, 0x79, 0x7e, 0x7d, 0x25,
	0x1b, 0x77, 0xbc, 0xf7, 0xc7, 0x0f, 0x84, 0x95, 0x10, 0x92, 0x67, 0x15, 0x11, 0x5a, 0x5e,
	0x41, 0x66, 0x0f, 0x38, 0x03, 0xb2, 0xf1, 0x5d, 0xf8, 0xab, 0xc0, 0x02, 0x76, 0x84, 0x28,
	0xf4, 0x9d, 0x56, 0x46, 0x60, 0x20, 0xdb, 0x68, 0xa7, 0xbb, 0xee, 0xac, 0x15, 0x01, 0x2f,
	0x20, 0x09, 0xdb, 0xc0, 0x16, 0xa1, 0x89, 0xf9, 0x94, 0x59, 0x00, 0xc1, 0x76, 0xbf, 0xc1,
	0x4d, 0x5d, 0x2d, 0xa9, 0x85, 0x2c, 0xd6, 0xd3, 0x14, 0xcc, 0x02, 0xc3, 0xc2, 0xfa, 0x6b,
	0xb7, 0xa6, 0xef, 0xdd, 0x12, 0x26, 0xa4, 0x63, 0xe3, 0x62, 0xbd, 0x56, 0x8a, 0x52, 0x2b,
	0xb9, 0xdf, 0x09, 0xbc, 0x0e, 0x97, 0xa9, 0xb0, 0x82, 0x46, 0x08, 0xd5, 0x1a, 0x8e, 0x1b,
	0xa7, 0x90, 0x98, 0xb9, 0xbb, 0x3c, 0x17, 0x9a, 0xf2, 0x82, 0xba, 0x64, 0x0a, 0x7f, 0xca,
	0x5a, 0x8c, 0x7c, 0xd3, 0x79, 0x09, 0x5b, 0x26, 0xbb, 0xbd, 0x25, 0xdf, 0x3d, 0x6f, 0x9a,
	0x8f, 0xee, 0x21, 0x66, 0xb0, 0x8d, 0x84, 0x4c, 0x91, 0x45, 0xd4, 0x77, 0x4f, 0xb3, 0x8c,
	0xbc, 0xa8, 0x99, 0xaa, 0x19, 0x53, 0x7c, 0x02, 0x87, 0xbb, 0x0b, 0x7c, 0x1a, 0x2d, 0xdf,
	0x48, 0x44, 0x06, 0xd6, 0x7d, 0x0c, 0x2d, 0x35, 0x76, 0xae, 0xc4, 0x5f, 0x71, 0x85, 0x97,
	0xc4, 0x3d, 0xef, 0x52, 0xbe, 0x00, 0xe4, 0xcd, 0x49, 0xd1, 0xd1, 0x1c, 0x3c, 0xd0, 0x1c,
	0x42, 0xaf, 0xd4, 0xbd, 0x58, 0x34, 0x07, 0x32, 0xee, 0xb9, 0xb5, 0xea, 0xff, 0xd7, 0x8c,
	0x0d, 0x2e, 0x2f, 0xaf, 0x87, 0xbb, 0xe6, 0x52, 0x71, 0x22, 0xf5, 0x25, 0x17, 0xa1, 0x82,
	0x04, 0xc2, 0x4a, 0xbd, 0x57, 0xc6, 0xab, 0xc8, 0x35, 0x0c, 0x3c, 0xd9, 0xc2, 0x43, 0xdb,
	0x27, 0x92, 0xcf, 0xb8, 0x25, 0x60, 0xfa, 0x21, 0x3b, 0x04, 0x52, 0xc8, 0x96, 0xba, 0x74,
	0xe3, 0x67, 0x3e, 0x8e, 0x8d, 0x61, 0x90, 0x92, 0x59, 0xb6, 0x1a, 0x1c, 0x5e, 0x21, 0xc1,
	0x65, 0xe5, 0xa6, 0x34, 0x05, 0x6f, 0xc5, 0x60, 0xb1, 0x83, 0xc1, 0xd5, 0xd5, 0xed, 0xd9,
	0xc7, 0x11, 0x7b, 0x49, 0x7a, 0xf9, 0xf9, 0x84, 0x47, 0x9b, 0xe2, 0xa5, 0x82, 0xe0, 0xc2,
	0x88, 0xd0, 0xb2, 0x58, 0x88, 0x7f, 0x45, 0x09, 0x67, 0x74, 0x61, 0xbf, 0xe6, 0x40, 0xe2,
	0x9d, 0xc2, 0x47, 0x05, 0x89, 0xed, 0xcb, 0xbb, 0xb7, 0x27, 0xe7, 0xdc, 0x7a, 0xfd, 0xbf,
	0xa8, 0xd0, 0xaa, 0x10, 0x39, 0x3c, 0x20, 0xf0, 0xd3, 0x6e, 0xb1, 0x72, 0xf8, 0xe6, 0x0f,
	0xef, 0x37, 0xe5, 0x09, 0x33, 0x5a, 0x83, 0x43, 0x80, 0x4f, 0x65, 0x2f, 0x7c, 0x8c, 0x6a,
	0xa0, 0x82, 0x0c, 0xd4, 0xd4, 0xfa, 0x81, 0x60, 0x3d, 0xdf, 0x06, 0xf1, 0x5f, 0x08, 0x0d,
	0x6d, 0x43, 0xf2, 0xe3, 0x11, 0x7d, 0x80, 0x32, 0xc5, 0xfb, 0xc5, 0xd9, 0x27, 0xec, 0xc6,
	0x4e, 0x65, 0x27, 0x76, 0x87, 0xa6, 0xee, 0xee, 0xd7, 0x8b, 0xd1, 0xa0, 0x5c, 0xb0, 0x42,
	0x13, 0x0e, 0x95, 0x4a, 0xf2, 0x06, 0xc6, 0x43, 0x33, 0xf4, 0xc7, 0xf8, 0xe7, 0x1f, 0xdd,
	0xe4, 0x46, 0x4a, 0x70, 0x39, 0x6c, 0xd0, 0xed, 0xca, 0xbe, 0x60, 0x3b, 0xd1, 0x7b, 0x57,
	0x48, 0xe5, 0x3a, 0x79, 0xc1, 0x69, 0x33, 0x53, 0x1b, 0x80, 0xb8, 0x91, 0x7d, 0xb4, 0xf6,
	0x17, 0x1a, 0x1d, 0x5a, 0x32, 0xd6, 0xcc, 0x71, 0x29, 0x3f, 0x28, 0xbb, 0xf3, 0x5e, 0x71,
	0xb8, 0x43, 0xaf, 0xf8, 0xb9, 0x64, 0xef, 0xc4, 0xa5, 0x6c, 0x08, 0x53, 0xc7, 0x00, 0x10,
	0x39, 0x4f, 0xdd, 0xe4, 0xb6, 0x19, 0x27, 0xfb, 0xb8, 0xf5, 0x32, 0x73, 0xe5, 0xcb, 0x32
};

/*
====================
COM_BlockSequenceCRCByte

For proxy protecting
====================
*/
byte COM_BlockSequenceCRCByte (byte *base, int length, int sequence)
{
	int		n;
	byte	*p;
	int		x;
	byte chkb[60 + 4];
	unsigned short crc;

	if (sequence < 0)
		Sys_Error ("sequence < 0, this shouldn't happen\n");

	p = chktbl + (sequence % (sizeof (chktbl) - 4));

	if (length > 60)
		length = 60;

	memcpy (chkb, base, length);

	chkb[length] = p[0];
	chkb[length+1] = p[1];
	chkb[length+2] = p[2];
	chkb[length+3] = p[3];

	length += 4;

	crc = CRC_Block (chkb, length);

	for (x = 0, n = 0; n < length; n++)
		x += chkb[n];

	crc = (crc ^ x) & 0xff;

	return crc;
}

//========================================================

float	frand (void)
{
	return (rand () & 32767) * (1.0 / 32767);
}

float	crand (void)
{
	return (rand () & 32767) * (2.0 / 32767) - 1;
}

/*
=================
Qcommon_ExecConfigs
=================
*/
void Qcommon_ExecConfigs (qboolean gameStartUp)
{
	// only when the game is first started we execute defaults.cfg
	if (gameStartUp)
		Cbuf_AddText ("exec defaults.cfg\n");

	// add in the game/mod config
	Cbuf_AddText ("exec config.cfg\n");

	if (gameStartUp)
	{
		// only when the game is first started we execute autoexec.cfg and set the cvars from commandline
		Cbuf_AddText ("exec autoexec.cfg\n");
		Cbuf_AddEarlyCommands (true);
	}

	Cbuf_Execute ();
}

// game given by user
char userGivenGame[MAX_QPATH];

void Key_Init (void);
void SCR_EndLoadingPlaque (void);

/*
=================
Qcommon_Init
=================
*/
void Qcommon_Init (int argc, char **argv)
{
	char *s;

	if (setjmp (abortframe))
		Sys_Error ("Error during initialization");

	z_chain.next = z_chain.prev = &z_chain;

	// prepare enough of the subsystems to handle
	// cvar and command buffer management
	COM_InitArgv (argc, argv);

	Cbuf_Init ();

	Cmd_Init ();
	Cvar_Init ();

	Key_Init ();

	// we need to add the early commands twice, because
	// a basedir needs to be set before execing
	// config files, but we want other parms to override
	// the settings of the config files
	Cbuf_AddEarlyCommands (false);
	Cbuf_Execute ();

	// remember the initial game name that might have been set on commandline
	{
		cvar_t * gameCvar = Cvar_Get("game", "", CVAR_LATCH | CVAR_SERVERINFO);
		char* game = "";
		if (gameCvar->string && gameCvar->string[0])
			game = gameCvar->string;
		Q_strlcpy (userGivenGame, game, sizeof(userGivenGame));
	}

	FS_InitFilesystem ();

	Qcommon_ExecConfigs (true);

	// init commands and vars
	Cmd_AddCommand ("z_stats", Z_Stats_f);

	host_speeds = Cvar_Get ("host_speeds", "0", 0);
	log_stats = Cvar_Get ("log_stats", "0", 0);
	developer = Cvar_Get ("developer", "0", 0);
	timescale = Cvar_Get ("timescale", "1", 0);
	fixedtime = Cvar_Get ("fixedtime", "0", 0);
	logfile_active = Cvar_Get ("logfile", "0", 0);
	showtrace = Cvar_Get ("showtrace", "0", 0);

#ifdef DEDICATED_ONLY
	dedicated = Cvar_Get ("dedicated", "1", CVAR_NOSET);
#else
	dedicated = Cvar_Get ("dedicated", "0", CVAR_NOSET);
#endif

	// For timing calculations
	cl_timedemo = Cvar_Get ("timedemo", "0", 0);
	cl_async = Cvar_Get ("cl_async", "1", CVAR_ARCHIVE);
	cl_maxfps = Cvar_Get ("cl_maxfps", "60", CVAR_ARCHIVE);
	r_maxfps = Cvar_Get ("r_maxfps", "95", CVAR_ARCHIVE);

	s = va ("%4.2f %s %s", VERSION, PLATFORM_STRING, __DATE__);
	Cvar_Get ("version", s, CVAR_SERVERINFO | CVAR_NOSET);

	if (dedicated->value)
		Cmd_AddCommand ("quit", Com_Quit);

	Sys_Init ();

	NET_Init ();
	Netchan_Init ();

	SV_Init ();
	CL_Init ();

	// add + commands from command line
	if (!Cbuf_AddLateCommands ())
	{
		// if the user didn't give any commands, run default action
		if (!dedicated->value)
			Cbuf_AddText ("d1\n");
		else
			Cbuf_AddText ("dedicated_start\n");

		Cbuf_Execute ();
	}
	else
	{
		// the user asked for something explicit
		// so drop the loading plaque
		SCR_EndLoadingPlaque ();
	}

	Com_Printf ("====== Engine Initialized ======\n\n");
}

/*
=================
Qcommon_Frame

Runs frames

A packetframe runs the server and the client, but not the renderer. The minimal interval of packetframes is about 10.000 microseconds.
NOTE: If packetframe run more often the movement prediction in pmove.c breaks.

A rendererframe runs the renderer, but not the client. The minimal interval is about 1000 microseconds.
=================
*/
#ifndef DEDICATED_ONLY
void Qcommon_Frame (int msec)
{
	char	*s;
	int		time_before, time_between, time_after;
	int		pfps; // target packet framerate.
	int		rfps; // target render framerate.
	static int packetdelta = 1000000; // time since last packetframe in microsec.
	static int renderdelta = 1000000; // time since last renderframe in microsec.
	static int clienttimedelta = 0; // accumulated time since last client run.
	static int servertimedelta = 0; // accumulated time since last server run.
	qboolean packetframe = true;
	qboolean renderframe = true;
	static int avgrenderframetime; // average time needed to process a render frame.
	static int renderframetimes[60];
	static qboolean last_was_renderframe;	
	static int avgpacketframetime; // average time needed to process a packet frame.
	static int packetframetimes[60];
	static qboolean last_was_packetframe;

	// an ERR_DROP was thrown, so get out of this frame
	if (setjmp (abortframe))
		return;

	if (log_stats->modified)
	{
		log_stats->modified = false;

		if (log_stats->value)
		{
			if (log_stats_file)
			{
				fclose (log_stats_file);
				log_stats_file = 0;
			}

			log_stats_file = fopen ("stats.log", "w");

			if (log_stats_file)
				fprintf (log_stats_file, "entities,dlights,parts,frame time\n");
		}
		else
		{
			if (log_stats_file)
			{
				fclose (log_stats_file);
				log_stats_file = 0;
			}
		}
	}

	// if recording an avi, lock to a fixed fps
#ifndef DEDICATED_ONLY
	extern cvar_t *cl_aviFrameRate;
	extern qboolean CL_VideoRecording(void);
	if (CL_VideoRecording() && cl_aviFrameRate->integer)
	{
		// fixed time for next frame
		msec = (int)ceil((1000.0f / cl_aviFrameRate->value) * timescale->value);
		if (msec == 0)
			msec = 1;
	}
#endif

	// timing debug
	if (fixedtime->value)
	{
		msec = (int)fixedtime->value;
	}
	else if (timescale->value)
	{
		msec *= timescale->value;
	}

	// content tracing debug
	if (showtrace->value)
	{
		extern	int c_traces, c_brush_traces;
		extern	int	c_pointcontents;

		Com_Printf ("%4i traces %4i points\n", c_traces, c_pointcontents);
		c_traces = 0;
		c_brush_traces = 0;
		c_pointcontents = 0;
	}

	// r_maxfps > 1000 breaks things, and so does <= 0
	// so cap to 1000 and treat <= 0 as "as fast as possible", which is 1000
	if (r_maxfps->value > 1000 || r_maxfps->value < 1)
	{
		Cvar_SetValue("r_maxfps", 1000);
	}

	if (cl_maxfps->value > 250)
	{
		Cvar_SetValue("cl_maxfps", 130);
	}
	else if (cl_maxfps->value < 1)
	{
		Cvar_SetValue("cl_maxfps", 60);
	}

	// save global time for network and input code
	curtime = Sys_Milliseconds();

	// calculate target packet and render framerate.
	if (VID_IsVSyncActive())
	{
		rfps = GFX_Core_GetRefreshRate ();
		if (rfps > r_maxfps->value)
		{
			rfps = (int)r_maxfps->value;
		}
	}
	else
	{
		rfps = (int)r_maxfps->value;
	}

	// The target render frame rate may be too high.
	// The current scene may be more complex than the previous one and SDL
	// may give us a 1 or 2 frames too low display refresh rate,
	// So add a security magin of 5%, e.g. 60fps * 0.95 = 57fps.
	pfps = (cl_maxfps->value > rfps) ? floor(rfps * 0.95) : cl_maxfps->value;

	// Calculate average time spend to process a render frame.
	// This is highly depended on the GPU and the scenes complexity.
	// Take the last 60 pure render frames (frames that are only render frames and nothing else)
	// into account and add a security margin of 2%.
	if (last_was_renderframe && !last_was_packetframe)
	{
		int measuredframes = 0;
		static int renderframenum;
		
		renderframetimes[renderframenum] = msec;
		
		for (int i = 0; i < 60; i++)
		{
			if (renderframetimes[i] != 0)
			{
				avgrenderframetime += renderframetimes[i];
				measuredframes++;
			}
		}
		
		avgrenderframetime /= measuredframes;
		avgrenderframetime += (avgrenderframetime * 0.02f);
		
		renderframenum++;
		
		if (renderframenum > 59)
		{
			renderframenum = 0;
		}
		
		last_was_renderframe = false;
	}
	
	// Calculate the average time spend to process a packet frame.
	// Packet frames are mostly dependend on the CPU speed and the network delay.
	// Take the last 60 pure packet frames (frames that are only packet frames and nothing else)
	// into account and add a security margin of 2%.
	if (last_was_packetframe && last_was_renderframe)
	{
		int measuredframes = 0;
		static int packetframenum;
		
		packetframetimes[packetframenum] = msec;
		
		for (int i = 0; i < 60; i++)
		{
			if (packetframetimes[i] != 0)
			{
				avgpacketframetime += packetframetimes[i];
				measuredframes++;
			}
		}
		
		avgpacketframetime /= measuredframes;
		avgpacketframetime += (avgpacketframetime * 0.02f);
		
		packetframenum++;
		
		if (packetframenum > 59)
		{
			packetframenum = 0;
		}
		
		last_was_packetframe = false;
	}

	// calculate timings.
	packetdelta += msec;
	renderdelta += msec;
	clienttimedelta += msec;
	servertimedelta += msec;

	if (!cl_timedemo->value)
	{
		if (cl_async->value)
		{
			// network frames..
			if (packetdelta < ((1000000.0f - avgpacketframetime) / pfps))
			{
				packetframe = false;
			}

			// render frames.
			if (renderdelta < ((1000000.0f - avgrenderframetime) / rfps))
			{
				renderframe = false;
			}
		}
		else
		{
			// cap frames at target framerate.
			if (renderdelta < (1000000.0f / rfps))
			{
				renderframe = false;
				packetframe = false;
			}
		}
	}
	else if (clienttimedelta < 1000 || servertimedelta < 1000)
	{
		return;
	}

	// dedicated server terminal console.
	do
	{
		s = Sys_ConsoleInput ();

		if (s)
			Cbuf_AddText (va("%s\n", s));
	} while (s);
	Cbuf_Execute ();

	if (host_speeds->value)
		time_before = Sys_Milliseconds ();

	// run the server frame
	if (packetframe) {
		SV_Frame (servertimedelta);
		servertimedelta = 0;
	}

	if (host_speeds->value)
		time_between = Sys_Milliseconds ();

	// run the client frame
	if (packetframe || renderframe) {
		CL_Frame(packetdelta, renderdelta, clienttimedelta, packetframe, renderframe);
		clienttimedelta = 0;
	}

	if (host_speeds->value)
		time_after = Sys_Milliseconds ();

	if (host_speeds->value)
	{
		int			all, sv, gm, cl, rf;

		all = time_after - time_before;
		sv = time_between - time_before;
		cl = time_after - time_between;
		gm = time_after_game - time_before_game;
		rf = time_after_ref - time_before_ref;
		sv -= gm;
		cl -= rf;
		Com_Printf ("all:%3i sv:%3i gm:%3i cl:%3i rf:%3i\n", all, sv, gm, cl, rf);
	}

	// reset deltas and mark frames if necessary.
	if (packetframe) {
		packetdelta = 0;
		last_was_packetframe = true;
	}

	if (renderframe) {
		renderdelta = 0;
		last_was_renderframe = true;
	}
}
#else
void Qcommon_Frame(int msec)
{
	char *s;
	int pfps; // target packetframerate
	static int packetdelta = 1000000; // time since last packetframe in microsec.
	static int servertimedelta = 0;	// accumulated time since last server run.
	qboolean packetframe = true;

	// an ERR_DROP was thrown, so get out of this frame
	if (setjmp(abortframe))
		return;

	// timing debug
	if (fixedtime->value)
	{
		msec = (int)fixedtime->value;
	}
	else if (timescale->value)
	{
		msec *= timescale->value;
	}

	// save global time for network and input code
	curtime = Sys_Milliseconds();

	// target framerate
	pfps = (int)cl_maxfps->value;

	// calculate timings.
	packetdelta += msec;
	servertimedelta += msec;

	// network frame time.
	if (packetdelta < (1000000.0f / pfps)) {
		packetframe = false;
	}

	// dedicated server terminal console.
	do {
		s = Sys_ConsoleInput();

		if (s)
			Cbuf_AddText(va("%s\n", s));
	} while (s);
	Cbuf_Execute();

	// run the serverframe.
	if (packetframe) {
		SV_Frame(servertimedelta);
		servertimedelta = 0;
	}

	// reset deltas if necessary.
	if (packetframe) {
		packetdelta = 0;
	}
}
#endif

/*
=================
Qcommon_Shutdown
=================
*/
void Qcommon_Shutdown (void)
{
	Cvar_Shutdown ();

	FS_Shutdown ();
}

/*
==============================================================================

COMMAND COMPLETION

==============================================================================
*/

// field we are working on, passed to Field_AutoComplete()
static field_t *completionField;

static char *completionString;
static char shortestMatch[MAX_TOKEN_CHARS];
static int	matchCount;

/*
==================
Field_Clear
==================
*/
void Field_Clear(field_t *edit)
{
	memset(edit->buffer, 0, MAX_EDIT_LINE);
	edit->cursor = 0;
}

/*
===============
Field_AutoComplete

Perform Tab expansion
===============
*/
void Field_AutoComplete(field_t *field)
{
	completionField = field;
	Field_CompleteCommand(completionField->buffer, true, true);
}

/*
===============
FindMatches
===============
*/
static void FindMatches(char *s)
{
	int		i;

	if (Q_stricmpn(s, completionString, strlen(completionString)))
		return;

	matchCount++;
	if (matchCount == 1)
	{
		Q_strlcpy(shortestMatch, s, sizeof(shortestMatch));
		return;
	}

	// cut shortestMatch to the amount common with s
	for (i = 0; shortestMatch[i]; i++)
	{
		if (i >= strlen(s))
		{
			shortestMatch[i] = 0;
			break;
		}

		if (tolower(shortestMatch[i]) != tolower(s[i]))
			shortestMatch[i] = 0;
	}
}

/*
===============
PrintMatches
===============
*/
static void PrintMatches(char *s)
{
	if (!Q_stricmpn(s, shortestMatch, strlen(shortestMatch)))
		Com_Printf("    %s\n", s);
}

/*
===============
PrintCvarMatches
===============
*/
static void PrintCvarMatches(char *s)
{
	if (!Q_stricmpn(s, shortestMatch, strlen(shortestMatch)))
		Com_Printf("    %s = \"%s\"\n", s, Cvar_VariableString(s));
}

/*
===============
FindFirstSeparator
===============
*/
static char *FindFirstSeparator(char *s)
{
	int i;

	for (i = 0; i < strlen(s); i++)
	{
		if (s[i] == ';')
			return &s[i];
	}

	return NULL;
}

/*
===============
Field_CompleteFilename
===============
*/
void Field_CompleteFilename(char *dir, char *ext, qboolean stripExt)
{
	int completionOffset;

	matchCount = 0;
	shortestMatch[0] = 0;

	FS_FilenameCompletion(dir, ext, stripExt, FindMatches);

	if (matchCount == 0)
		return;

	completionOffset = strlen(completionField->buffer) - strlen(completionString);

	Q_strlcpy(&completionField->buffer[completionOffset], shortestMatch, sizeof(completionField->buffer) - completionOffset);
	completionField->cursor = strlen(completionField->buffer) + 1;

	if (matchCount == 1)
	{
		strcat(completionField->buffer, " ");
		completionField->cursor++;
		return;
	}

	Com_Printf("]%s\n", completionField->buffer);

	FS_FilenameCompletion(dir, ext, stripExt, PrintMatches);
}

/*
===============
Field_CompleteCommand
===============
*/
void Field_CompleteCommand(char *cmd, qboolean doCommands, qboolean doCvars)
{
	char		*p;
	int			completionArgument = 0;

	// skip leading whitespace and quotes
	cmd = Com_SkipCharset(cmd, " \"");

	Cmd_TokenizeString(cmd, false);
	completionArgument = Cmd_Argc();

	// if there is trailing whitespace on the cmd
	if (*(cmd + strlen(cmd) - 1) == ' ')
	{
		completionString = "";
		completionArgument++;
	}
	else
	{
		completionString = Cmd_Argv(completionArgument - 1);
	}

	if (completionArgument > 1)
	{
		if ((p = FindFirstSeparator(cmd)))
		{
			// compound command
			Field_CompleteCommand(p + 1, true, true);
		}
		else
		{
			char *baseCmd = Cmd_Argv(0);

			// FIXME: all this junk should really be associated with the respective
			// commands, instead of being hard coded here
			if ((!Q_stricmp(baseCmd, "map") || !Q_stricmp(baseCmd, "gamemap")) && completionArgument == 2)
			{
				Field_CompleteFilename("maps", "bsp", true);
			}
			else if ((!Q_stricmp(baseCmd, "exec") || !Q_stricmp(baseCmd, "writeconfig")) && completionArgument == 2)
			{
				Field_CompleteFilename("", "cfg", false);
			}
			else if (!Q_stricmp(baseCmd, "condump") && completionArgument == 2)
			{
				Field_CompleteFilename("", "txt", false);
			}
			else if (!Q_stricmp(baseCmd, "demomap") && completionArgument == 2)
			{
				Field_CompleteFilename("demos", "dm2", false);
			}
		}
	}
	else
	{
		int completionOffset;

		if (completionString[0] == '\\' || completionString[0] == '/')
			completionString++;

		matchCount = 0;
		shortestMatch[0] = 0;

		if (strlen(completionString) == 0)
			return;

		// find matches
		if (doCommands)
			Cmd_CommandCompletion(FindMatches);
		if (doCvars)
			Cvar_CommandCompletion(FindMatches);

		if (matchCount == 0)
			return;	// no matches

		completionOffset = strlen(completionField->buffer) - strlen(completionString);
	
		Q_strlcpy(&completionField->buffer[completionOffset], shortestMatch, sizeof(completionField->buffer) - completionOffset);
		completionField->cursor = strlen(completionField->buffer) + 1;

		if (matchCount == 1)
		{
			strcat(completionField->buffer, " ");
			completionField->cursor++;
			return;
		}

		Com_Printf("]%s\n", completionField->buffer);

		// run through again, printing matches
		if (doCommands)
			Cmd_CommandCompletion(PrintMatches);
		if (doCvars)
			Cvar_CommandCompletion(PrintCvarMatches);
	}
}
