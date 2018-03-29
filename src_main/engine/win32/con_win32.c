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

#include "qcommon.h"

#include <windows.h>

#define QCONSOLE_THEME FOREGROUND_RED | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE
#define QCONSOLE_INPUT_RECORDS 1024

// used to track key input
static int qconsole_chars = 0;

// used to restore original color theme 
static int qconsole_orig_attrib;

/*
==================
CON_Hide
==================
*/
void CON_Hide (void)
{
}

/*
==================
CON_Show
==================
*/
void CON_Show (void)
{	
}

/*
==================
CON_Shutdown
==================
*/
void CON_Shutdown (void)
{
	HANDLE hout;
	COORD screen = { 0, 0 };
	DWORD written;
	
	hout = GetStdHandle(STD_OUTPUT_HANDLE);
	
	SetConsoleTextAttribute(hout, qconsole_orig_attrib);
	FillConsoleOutputAttribute(hout, qconsole_orig_attrib, 63999, screen, &written);
}

/*
==================
CON_Init
==================
*/
void CON_Init (void)
{
	HANDLE hout;
	COORD screen = { 0, 0 };
	DWORD written, read;
	CONSOLE_SCREEN_BUFFER_INFO binfo;
	SMALL_RECT rect;
	WORD oldattrib;
	
	hout = GetStdHandle(STD_OUTPUT_HANDLE);

	// remember original color theme
	ReadConsoleOutputAttribute(hout, &oldattrib, 1, screen, &read);
	qconsole_orig_attrib = oldattrib;
	
	SetConsoleTitle("Dedicated Server Console");

	SetConsoleTextAttribute(hout, QCONSOLE_THEME);
	FillConsoleOutputAttribute(hout, QCONSOLE_THEME, 63999, screen, &written);
	
	// adjust console scroll to match up with cursor position  
	GetConsoleScreenBufferInfo(hout, &binfo);
	rect.Top = binfo.srWindow.Top;
	rect.Left = binfo.srWindow.Left;
	rect.Bottom = binfo.srWindow.Bottom;
	rect.Right = binfo.srWindow.Right;
	rect.Top += (binfo.dwCursorPosition.Y - binfo.srWindow.Bottom);
	rect.Bottom = binfo.dwCursorPosition.Y;
	SetConsoleWindowInfo(hout, TRUE, &rect);
}

/*
==================
CON_ConsoleInput
==================
*/
char *CON_ConsoleInput (void)
{
	HANDLE hin, hout;
	INPUT_RECORD buff[QCONSOLE_INPUT_RECORDS];
	DWORD count = 0;
	int i;
	static char input[1024] = { "" };
	int inputlen;
	int newlinepos = -1;
	CHAR_INFO line[QCONSOLE_INPUT_RECORDS];
	int linelen = 0;

	inputlen = 0;
	input[0] = '\0';

	hin = GetStdHandle(STD_INPUT_HANDLE);
	if (hin == INVALID_HANDLE_VALUE)
		return NULL;
	hout = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hout == INVALID_HANDLE_VALUE)
		return NULL;

	if (!PeekConsoleInput(hin, buff, QCONSOLE_INPUT_RECORDS, &count))
		return NULL;

	// if we have overflowed, start dropping oldest input events
	if (count == QCONSOLE_INPUT_RECORDS)
	{
		ReadConsoleInput(hin, buff, 1, &count);
		return NULL;
	}

	for (i = 0; i < count; i++)
	{
		if (buff[i].EventType == KEY_EVENT && buff[i].Event.KeyEvent.bKeyDown)
		{
			if (buff[i].Event.KeyEvent.wVirtualKeyCode == VK_RETURN)
			{
				newlinepos = i;
				break;
			}

			if (linelen < QCONSOLE_INPUT_RECORDS &&	buff[i].Event.KeyEvent.uChar.AsciiChar)
			{
				if (buff[i].Event.KeyEvent.wVirtualKeyCode == VK_BACK)
				{
					if (linelen > 0)
						linelen--;

				}
				else
				{
					line[linelen].Attributes = QCONSOLE_THEME;
					line[linelen++].Char.AsciiChar = buff[i].Event.KeyEvent.uChar.AsciiChar;
				}
			}
		}
	}

	// provide visual feedback for incomplete commands
	if (linelen != qconsole_chars)
	{
		CONSOLE_SCREEN_BUFFER_INFO binfo;
		COORD writeSize = { QCONSOLE_INPUT_RECORDS, 1 };
		COORD writePos = { 0, 0 };
		SMALL_RECT writeArea = { 0, 0, 0, 0 };
		int i;

		// keep track of this so we don't need to re-write to console every frame
		qconsole_chars = linelen;

		GetConsoleScreenBufferInfo(hout, &binfo);

		// adjust scrolling to cursor when typing
		if (binfo.dwCursorPosition.Y > binfo.srWindow.Bottom)
		{
			SMALL_RECT rect;

			rect.Top = binfo.srWindow.Top;
			rect.Left = binfo.srWindow.Left;
			rect.Bottom = binfo.srWindow.Bottom;
			rect.Right = binfo.srWindow.Right;

			rect.Top += (binfo.dwCursorPosition.Y - binfo.srWindow.Bottom);
			rect.Bottom = binfo.dwCursorPosition.Y;

			SetConsoleWindowInfo(hout, TRUE, &rect);
			GetConsoleScreenBufferInfo(hout, &binfo);
		}

		writeArea.Left = 0;
		writeArea.Top = binfo.srWindow.Bottom;
		writeArea.Bottom = binfo.srWindow.Bottom;
		writeArea.Right = QCONSOLE_INPUT_RECORDS;

		// pad line with ' ' to handle VK_BACK
		for (i = linelen; i < QCONSOLE_INPUT_RECORDS; i++)
		{
			line[i].Char.AsciiChar = ' ';
			line[i].Attributes = QCONSOLE_THEME;
		}

		if (linelen > binfo.srWindow.Right)
		{
			WriteConsoleOutput(hout, line + (linelen - binfo.srWindow.Right), writeSize, writePos, &writeArea);
		}
		else
		{
			WriteConsoleOutput(hout, line, writeSize, writePos, &writeArea);
		}

		if (binfo.dwCursorPosition.X != linelen)
		{
			COORD cursorPos = { 0, 0 };

			cursorPos.X = linelen;
			cursorPos.Y = binfo.srWindow.Bottom;
			SetConsoleCursorPosition(hout, cursorPos);
		}
	}

	// don't touch the input buffer if this is an incomplete command
	if (newlinepos < 0)
	{
		return NULL;
	}
	else
	{
		// add a newline
		COORD cursorPos = { 0, 0 };
		CONSOLE_SCREEN_BUFFER_INFO binfo;

		GetConsoleScreenBufferInfo(hout, &binfo);
		cursorPos.Y = binfo.srWindow.Bottom + 1;
		SetConsoleCursorPosition(hout, cursorPos);
	}

	if (!ReadConsoleInput(hin, buff, newlinepos + 1, &count))
		return NULL;

	for (i = 0; i < count; i++)
	{
		if (buff[i].EventType == KEY_EVENT && buff[i].Event.KeyEvent.bKeyDown)
		{
			if (buff[i].Event.KeyEvent.wVirtualKeyCode == VK_BACK)
			{
				if (inputlen > 0)
					input[--inputlen] = '\0';
				continue;
			}
			if (inputlen < (sizeof(input) - 1) && buff[i].Event.KeyEvent.uChar.AsciiChar)
			{
				input[inputlen++] = buff[i].Event.KeyEvent.uChar.AsciiChar;
				input[inputlen] = '\0';
			}
		}
	}

	if (!inputlen)
		return NULL;

	return input;
}

/*
==================
CON_Print
==================
*/
void CON_Print(char *string)
{
	fputs(string, stderr);
}
