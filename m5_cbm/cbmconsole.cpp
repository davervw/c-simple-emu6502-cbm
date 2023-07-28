// cbmconsole.c - Commodore Console Emulation
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C Portable Version)
// C64/6502 Emulator for M5Stack Cores
//
// MIT License
//
// Copyright(c) 2020 by David R.Van Wagner
// davevw.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

#include "M5Core.h"
#include "cbmconsole.h"

// locals
static int supress_first_clear = 1;
static int supress_next_cr = 0;
static char buffer[256];
static int buffer_head = 0;
static int buffer_tail = 0;
static int buffer_count = 0;

static void Console_Clear()
{
	if (supress_first_clear)
	{
		supress_first_clear = 0;
		return;
	}
	M5Serial.print("\x1B[2J\x1B[H");
}

static void Console_Cursor_Up()
{
	M5Serial.print("\x1B[A");
}

static void Console_Cursor_Down()
{
	M5Serial.print("\x1B[B");
}

static void Console_Cursor_Left()
{
	M5Serial.print("\x1B[D");
}

static void Console_Cursor_Right()
{
	M5Serial.print("\x1B[C");
}

static void Console_Cursor_Home()
{
	M5Serial.print("\x1B[H");
}

void CBM_Console_WriteChar(unsigned char c)
{
	// we're emulating, so draw character on local console window
	if (c == 0x0D)
	{
		if (supress_next_cr)
			supress_next_cr = 0;
		else
			M5Serial.print('\n');
	}
	else if (c >= ' ' && c <= '~')
	{
		//ApplyColor ? .Invoke();
		M5Serial.write(c);
	}
	else if (c == 157) // left
		Console_Cursor_Left();
	else if (c == 29) // right
		Console_Cursor_Right();
	else if (c == 145) // up
		Console_Cursor_Up();
	else if (c == 17) // down
		Console_Cursor_Down();
	else if (c == 19) // home
		Console_Cursor_Home();
	else if (c == 147)
	{
		Console_Clear();
	}
}

// blocking read to get next typed character
unsigned char CBM_Console_ReadChar(void)
{
	unsigned char c;
	if (buffer_count == 0)
	{
    while (true)
    {
		  int i = M5Serial.read();
      if (i != -1)
      {
        c = i;
        break;
      }
    }
//		if (c >= ' ' && c <= '~')
//			M5Serial.Write(c); // echo
		if (c == '\r')
			supress_next_cr = true;
		return c;
	}
	c = buffer[buffer_head++];
	if (buffer_head >= sizeof(buffer))
		buffer_head = 0;
	--buffer_count;
	return c;
}

void CBM_Console_Push(const char* s)
{
	while (s != 0 && *s != 0 && buffer_count < sizeof(buffer))
	{
		buffer[buffer_tail++] = *(s++);
		if (buffer_tail >= sizeof(buffer))
			buffer_tail = 0;
		++buffer_count;
	}
}
