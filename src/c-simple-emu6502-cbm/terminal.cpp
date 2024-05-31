// terminal.cpp - dumb terminal for LCD and Windows
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C Portable Version)
// C64/6502 Unified Emulator for M5Stack/Teensy/ESP32 LCDs and Windows
//
// MIT License
//
// Copyright (c) 2024 by David R. Van Wagner
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

#include "ASCIIKeyboard.h"
#include "emucbm.h"
#include "terminal.h"

#ifdef _WINDOWS
#include <memory.h>
#include "WindowsDraw.h"
#include "WindowsTime.h"
#else // NOT _WINDOWS
#include "config.h"
#include "LCDDraw.h"
#endif // NOT _WINDOWS


#ifdef M5STACK
const int cols = 40;
const int rows = 30;
#endif
#ifdef _WINDOWS
const int cols = 80;
const int rows = 25;
#endif
#ifdef ILI9341
const int cols = 40;
const int rows = 25;
#endif
#ifdef ILI9488
const int cols = 60;
const int rows = 25;
#endif
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
const int cols = 40;
const int rows = 25;
#endif
#ifdef ARDUINO_SUNTON_8048S070
const int cols = 80;
const int rows = 25;
#endif

Terminal::CRNLMODE Terminal::crnlmode = NEWLINE_ONLY;

Terminal::Terminal()
{
	keyboard = new ASCIIKeyboard();
	chargen = new byte[128 * 8];
	EmuCBM::File_ReadAllBytes(chargen, 128 * 8, "/roms/minimum/asciifont.bin");
	videoBuffer = new char[rows * cols];
	clearScreen();
#ifdef _WINDOWS
	redrawRequiredSignal = false;
	needsPaintFrame = false;
	lastPaintFrame = micros();
	WindowsDraw::CreateRenderTarget(cols*8, rows*8, 32, 16, redrawRequiredSignal);
	WindowsDraw::BeginDraw();
#else // NOT _WINDOWS
#ifdef M5STACK
  LCDDraw::CreateRenderTarget(cols*8, rows*8, 8, 8);
#endif  
#ifdef ARDUINO_SUNTON_8048S070
  LCDDraw::CreateRenderTarget(cols*8, rows*8, 8, 16);
#endif
#ifdef ILI9341
  LCDDraw::CreateRenderTarget(cols*8, rows*8, 4, 8);
#endif    
#ifdef ILI9488    
  LCDDraw::CreateRenderTarget(cols*8, rows*8, 8, 12);
#endif   
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
  LCDDraw::CreateRenderTarget(cols*8, rows*8, 8, 6);
#endif
#endif // _WINDOWS  
}

Terminal::~Terminal()
{
	delete keyboard;
	delete[] videoBuffer;
#ifdef _WINDOWS  
	WindowsDraw::EndDraw();
#endif  
}

void Terminal::write(char c)
{
	write_internal(' ');
	backspace();
	write_internal(c);
	write_internal('_'); // TODO: proper cursor, blink, refresh
	backspace();
}

void Terminal::write_internal(char c)
{
	if (c < 0)
		return;
	if (c == '\r')
	{
		if (crnlmode != NEWLINE_ONLY)
			carriagereturn();
		if (crnlmode == CARRIAGE_RETURN_ONLY)
			newline();
		return;
	}
	else if (c == '\n')
	{
		if (crnlmode == NEWLINE_ONLY)
			carriagereturn();
		if (crnlmode != CARRIAGE_RETURN_ONLY)
			newline();
		return;
	}
	else if (c == '\b')
	{
		backspace();
		return;
	}
	else if (c == 12) {
		clearScreen();
		return;
	}
	byte* image = &chargen[c * 8];
	videoBuffer[y * cols + x] = c;
#ifdef _WINDOWS
	if (!redrawRequiredSignal) // otherwise update delayed to update entire screen
		WindowsDraw::DrawCharacter2Color(image, x, y, 255, 255, 255, 0, 0, 0);
	needsPaintFrame = true;
#else // NOT _WINDOWS
  LCDDraw::DrawCharacter2Color(image, x, y, 0xFFFF, 0x0000);
#endif // NOT _WINDOWS  
	if (++x == cols) {
		x = 0;
		newline();
		carriagereturn();
	}
}

bool Terminal::read(char& c)
{
	return keyboard->read(c);
}

bool Terminal::readWaiting()
{
	return keyboard->readWaiting();
}

void Terminal::write(const char* s)
{
	if (s == 0)
		return;
	while (*s != 0)
		write(*(s++));
}

void Terminal::clearScreen()
{
	home();
	memset(videoBuffer, ' ', (size_t)rows * cols);
#ifdef _WINDOWS
	redrawRequiredSignal = true;
#else
  LCDDraw::ClearAll(0);  
#endif  
}

void Terminal::home()
{
	x = 0;
	y = 0;
}

void Terminal::carriagereturn()
{
	x = 0;
}

void Terminal::newline()
{
	if (++y == rows)
	{
		scrollup();
		y = rows - 1;
	}
}

void Terminal::scrollup()
{
	for (int row = 1; row < rows; ++row)
		memcpy(&videoBuffer[(row - 1) * cols], &videoBuffer[row * cols], cols);
	memset(&videoBuffer[(rows - 1) * cols], ' ', cols);
#ifdef _WINDOWS  
	redrawRequiredSignal = true;
#else
  RedrawScreen();
#endif  
}

void Terminal::backspace()
{
	if (--x < 0) {
		x = cols-1;
		if (--y < 0)
			y = 0;
	}
}

#ifdef _WINDOWS
void Terminal::CheckPaintFrame(unsigned long micros_now)
{
	if (redrawRequiredSignal || needsPaintFrame && (micros_now - lastPaintFrame) >= paintFrameInterval)
	{
		if (redrawRequiredSignal) {
			redrawRequiredSignal = false;
			WindowsDraw::ClearScreen(0, 0, 0);
			RedrawScreen();
		}
		WindowsDraw::EndDraw(); // will paint anything pending
		WindowsDraw::BeginDraw(); // start a new session
		lastPaintFrame = micros_now;
		needsPaintFrame = false;
	}
}
#endif // _WINDOWS

void Terminal::RedrawScreen()
{
	char* p = videoBuffer;
	for (int row = 0; row < rows; ++row) {
		for (int col = 0; col < cols; ++col) {
			char c = *(p++);
			byte* image = &chargen[c * 8];
#ifdef _WINDOWS      
			WindowsDraw::DrawCharacter2Color(image, col, row, 255, 255, 255, 0, 0, 0);
#else // NOT _WINDOWS
			LCDDraw::DrawCharacter2Color(image, col, row, 0xFFFF, 0x0000);
#endif // NOT _WINDOWS
		}
	}
}
