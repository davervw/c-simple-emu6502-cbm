// terminal.h - dumb terminal for LCD and Windows
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

#pragma once

#ifdef _WINDOWS
#include <WindowsDraw.h>
#else // NOT _WINDOWS
#include "config.h"
#endif // NOT _WINDOWS

#include "ASCIIKeyboard.h"
#include "ICharInput.h"
#include "ICharOutput.h"

// This diagram illustrates the intended use... but serial interface is not required
//
//   External interface is virtualized using ICharInput, ICharOutput interfaces
//      reads will get from keyboard
//      writes will go to screen
//      outside caller is responsible to do reads/writes and pass reads to serial or other
//         good news is that the MC6850 object can be hooked to the terminal input and output to satisify that
//         and due to abstraction you are not tied to using the MC6850
//      outside caller is responsible for ocassionally calling CheckPaintFrame() on Windows,
//         recommended around 60fps, more often will be throttled to 60fps
//   Keyboard is integrated
//   Screen is integrated
//
//          ___________                                          mos6502=||
//         /          /                                              rom=||
//        /  Screen  /                                               ram=||
//       /_________ /  <<<<                        ___  ___              ||
//      / ++ ++ ++ /   .... serial interface ... __| |__| |__ ... mc6850=||
//     / Keyboard /    >>>>                         
//    / ++ ++ ++ /                                  
//   -----------
//
//  Features
//  * 80x25 screen (can be configured bigger on Windows)
//  * newline
//  * scrolling
//  * buffered screen memory with redraw on Windows
//  * line wrapping
//  * backspace
//

class Terminal : public ICharInput, public ICharOutput
{
public:
	Terminal();
	virtual ~Terminal();
	virtual void write(char c);
	virtual bool read(char& c);
	virtual bool readWaiting();
	void write(const char* s);
	void clearScreen();
	bool SaveState(byte*& state, size_t& size);
	bool RestoreState(byte* state, size_t size);

	typedef enum _CRNLMODE {
		CARRIAGE_RETURN_AND_NEWLINE = 0,
		CARRIAGE_RETURN_ONLY = 1, // treat as carriage return + newline
		NEWLINE_ONLY = 2, // treat as carriage return + newline
	} CRNLMODE;

	static CRNLMODE crnlmode; // default NEWLINE_ONLY; // TODO: for input too

	byte specialKey; // allow special processing outside of normal channels, e.g. load/save state

private:
	ASCIIKeyboard* keyboard;
	int x, y;
	byte* chargen;
	char* videoBuffer;

	void home();
	void carriagereturn();
	void newline();
	void upline();
	void backspace();
	void scrollup();
	void write_internal(char c);
 	void RedrawScreen();

#ifdef _WINDOWS
public:
	void CheckPaintFrame(unsigned long micros_now);
private:
	bool needsPaintFrame;
	unsigned long lastPaintFrame;
	static const long paintFrameInterval = 1000000 / 60; // TODO: have LCDs employ this technique for more optimal screen refreshes (screen scrolling, and other high rate updates)
	bool redrawRequiredSignal;
#endif // WINDOWS
};
