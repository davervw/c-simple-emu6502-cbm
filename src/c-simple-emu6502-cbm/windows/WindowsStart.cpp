// WindowsStart.cpp - starts Commodore emulators for Windows
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

#ifdef _WINDOWS
#include "../emutest.h"
#include "../emu6502.h"
#include "../emuc64.h"
#include "../emuvic20.h"
#include "../emuc128.h"
#include "../emumin.h"
#include "WindowsStart.h"

int main_go_num = 64;
const char* StartupPRG = 0;
HWND hWnd;

void WindowsStart(HWND hWnd, bool& shuttingDown) // TODO: common start for Windows and Arduino
{
	::hWnd = hWnd;

	while (true)
	{
		Emu6502* system = 0;
		switch (main_go_num)
		{
			case 0: 
				Terminal::crnlmode = Terminal::NEWLINE_ONLY;
				system = new EmuTest(); 
				break;
			case 1:
				Terminal::crnlmode = Terminal::CARRIAGE_RETURN_ONLY;
				system = new EmuMinimum(0xFFF8);
				break;
			case 20: 
				system = new EmuVic20(5); 
				break;
			case 128:
				system = new EmuC128();
				break;
			case 64: 
			default: 
				system = new EmuC64(); break;
		}
		system->ResetRun();
		delete system;
	}
}
#endif // _WINDOWS
