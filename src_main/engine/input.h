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
// input.h -- external (non-keyboard) input devices

extern int sys_frame_time; // Saves the time of the last input event.

/*
* Initializes the input backend
*/
void IN_Init(void);

/*
* Move handling
*/
void IN_Move(usercmd_t *cmd);

/*
* Shuts the backend down
*/
void IN_Shutdown(void);

/*
* Updates the state of the input queue
*/
void IN_Update(void);

/*
* Removes all pending events from SDLs queue.
*/
void In_FlushQueue(void);
