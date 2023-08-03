// cbmconsole.cpp - Commodore Console Emulation
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C Portable Version)
// C64/6502 Emulator for Microsoft Windows Console
//
// MIT License
//
// Copyright (c) 2023 by David R. Van Wagner
// davevw.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
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
//
// Console cls(), PERR derived or from https://support.microsoft.com/en-au/help/99261/how-to-performing-clear-screen-cls-in-a-console-application
//
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#endif

int supress_first_clear = 1;
static bool supress_next_home = false;

static unsigned char buffer[256];
int buffer_head = 0;
int buffer_tail = 0;
int buffer_count = 0;

#ifdef WIN32
static void cls(HANDLE hConsole);

// From https://support.microsoft.com/en-au/help/99261/how-to-performing-clear-screen-cls-in-a-console-application
/* Standard error macro for reporting API errors */
#define PERR(bSuccess, api){if(!(bSuccess)) printf("%s:Error %d from %s on line %d\n", __FILE__, GetLastError(), api, __LINE__);}
#endif

static void Console_Clear()
{
   if (supress_first_clear)
   {
      supress_first_clear = 0;
      return;
   }

#ifdef WIN32
   // See https://docs.microsoft.com/en-us/windows/console/getstdhandle
   HANDLE hStdout;
   hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

   cls(hStdout);
#else
   printf("\x1B[2J\x1B[H");
#endif
}

#ifdef WIN32
BOOL Console_GetCursor(HANDLE hStdout, COORD* coord)
{
   CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */
   BOOL bSuccess = GetConsoleScreenBufferInfo(hStdout, &csbi);
   PERR(bSuccess, "GetConsoleScreenBufferInfo");
   coord->X = csbi.dwCursorPosition.X;
   coord->Y = csbi.dwCursorPosition.Y;
   return bSuccess;
}

void Console_SetCursor(HANDLE hStdout, COORD coord)
{
   BOOL bSuccess = SetConsoleCursorPosition(hStdout, coord);
   PERR(bSuccess, "SetConsoleCursorPosition");
}
#endif

static void Console_Cursor_Up()
{
#ifdef WIN32
   HANDLE hStdout;
   hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   COORD coord = { 0, 0 };
   if (Console_GetCursor(hStdout, &coord) && coord.Y > 0)
   {
      --coord.Y;
      Console_SetCursor(hStdout, coord);
   }
#else
   printf("\x1B[A");
#endif
}

static void Console_Cursor_Down()
{
#ifdef WIN32
   HANDLE hStdout;
   hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   COORD coord = { 0, 0 };
   if (Console_GetCursor(hStdout, &coord))
   {
      SHORT x = coord.X; // save column
      putchar('\n');
      if (Console_GetCursor(hStdout, &coord))
      {
         coord.X = x; // restore column
         Console_SetCursor(hStdout, coord);
      }
   }
#else
   printf("\x1B[B");
#endif
}

static void Console_Cursor_Left()
{
#ifdef WIN32
   HANDLE hStdout;
   hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   COORD coord = { 0, 0 };
   if (Console_GetCursor(hStdout, &coord) && coord.X > 0)
      putchar('\b');
   else if (coord.Y > 0)
   {
      CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */
      BOOL bSuccess = GetConsoleScreenBufferInfo(hStdout, &csbi);
      PERR(bSuccess, "ConsoleScreenBufferInfo");
      if (bSuccess)
      {
         --coord.Y;
         coord.X = csbi.dwSize.X - 1;
         Console_SetCursor(hStdout, coord);
      }
   }
#else
   printf("\x1B[D");
#endif
}

static void Console_Cursor_Right()
{
#ifdef WIN32
   HANDLE hStdout;
   hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */
   COORD coord = { 0, 0 };
   BOOL bSuccess = GetConsoleScreenBufferInfo(hStdout, &csbi);
   PERR(bSuccess, "ConsoleScreenBufferInfo");
   if (bSuccess && Console_GetCursor(hStdout, &coord))
   {
      if (coord.X < csbi.dwSize.X - 1)
      {
         ++coord.X;
         Console_SetCursor(hStdout, coord);
      }
      else
         putchar('\n');
   }
#else
   printf("\x1B[C");
#endif
}

static void Console_Cursor_Home()
{
    if (supress_next_home)
    {
        supress_next_home = false;
        return;
    }
#ifdef WIN32
   HANDLE hStdout;
   hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   COORD coord = { 0, 0 };
   Console_SetCursor(hStdout, coord);
#else
   printf("\x1B[H");
#endif
}

static int ReverseActive = 0;

static void Console_Reverse_On()
{
   if (!ReverseActive)
   {
      printf("\x1B[7m");
      ReverseActive = 1;
	}
}

static void Console_Reverse_Off()
{
   if (ReverseActive)
   {
      printf("\x1B[m");
      ReverseActive = 0;
   }
}

extern void CBM_Console_WriteChar(unsigned char c, bool supress_next_home)
{
    if (supress_next_home)
        ::supress_next_home = true;

   // we're emulating, so draw character on local console window
   if (c == 0x0D)
   {
      putchar('\n');
      Console_Reverse_Off();
   }
   else if (c >= ' ' && c <= '~')
   {
      //ApplyColor ? .Invoke();
      putchar(c);
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
      Console_Clear();
   else if (c == 18)
      Console_Reverse_On();
   else if (c == 146)
      Console_Reverse_Off();
}

// blocking read to get next typed character
extern unsigned char CBM_Console_ReadChar(void)
{
   if (buffer_count == 0)
   {
      // System.Console.ReadLine() has features of history (cursor up/down, F7/F8), editing (cursor left/right, delete, backspace, etc.)
      //ApplyColor ? .Invoke();
      while (1)
      {
         fgets((char*)& buffer[0], sizeof(buffer) - 1, stdin); // save room for carriage return and null
         buffer[strlen((char*)buffer)-1] = '\r'; // replace newline
         buffer_head = 0;
         buffer_tail = buffer_count = (int)strlen((char*)buffer);
         Console_Cursor_Up();
         break;
      }
   }
   unsigned char c = buffer[buffer_head++];
   if (buffer_head >= sizeof(buffer))
      buffer_head = 0;
   --buffer_count;
   return c;
}

extern void CBM_Console_Push(const char* s)
{
   while (s != 0 && *s != 0 && buffer_count < sizeof(buffer))
   {
      buffer[buffer_tail++] = *(s++);
      if (buffer_tail >= sizeof(buffer))
         buffer_tail = 0;
      ++buffer_count;
   }
}

#ifdef WIN32
// borrowed from https://support.microsoft.com/en-au/help/99261/how-to-performing-clear-screen-cls-in-a-console-application
static void cls(HANDLE hConsole)
{
   COORD coordScreen = { 0, 0 };    /* here's where we'll home the
                              cursor */
   BOOL bSuccess;
   DWORD cCharsWritten;
   CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */
   DWORD dwConSize;                 /* number of character cells in
                              the current buffer */

                              /* get the number of character cells in the current buffer */

   bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
   PERR(bSuccess, "GetConsoleScreenBufferInfo");
   dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

   /* fill the entire screen with blanks */

   bSuccess = FillConsoleOutputCharacter(hConsole, (TCHAR) ' ',
      dwConSize, coordScreen, &cCharsWritten);
   PERR(bSuccess, "FillConsoleOutputCharacter");

   /* get the current text attribute */

   bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
   PERR(bSuccess, "ConsoleScreenBufferInfo");

   /* now set the buffer's attributes accordingly */

   bSuccess = FillConsoleOutputAttribute(hConsole, csbi.wAttributes,
      dwConSize, coordScreen, &cCharsWritten);
   PERR(bSuccess, "FillConsoleOutputAttribute");

   /* put the cursor at (0, 0) */

   bSuccess = SetConsoleCursorPosition(hConsole, coordScreen);
   PERR(bSuccess, "SetConsoleCursorPosition");
   return;
}
#endif
