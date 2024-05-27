// VDC8563 - 80 column video display chip on C128 (and VDC8568 on C128D)
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

class VDC8563
{
private:
	byte* registers;
	byte* vdc_ram;
	byte register_addr;
	byte data;
	bool ready = false;

public:
	VDC8563();
	~VDC8563();

	bool active = false;

	byte GetAddressRegister();
	void SetAddressRegister(byte value);
	byte GetDataRegister();
	void SetDataRegister(byte value);
	void Activate();
	void Deactivate();
	int VDCColorToLCDColor(byte value);
	void DrawChar(byte c, int col, int row, int fg, int bg, byte attrib);
	void DrawChar(int offset);
	void RedrawScreen();
	// void BlinkCursor();
	// void HideCursor();
	// void ShowCursor();

#ifdef _WINDOWS
public:
	void CheckPaintFrame(unsigned long micros_now);
private:
	bool needsPaintFrame;
	unsigned long lastPaintFrame;
	static const long paintFrameInterval = 1000000 / 60; // TODO: have LCDs employ this technique for more optimal screen refreshes (screen scrolling, and other high rate updates)
	bool redrawRequiredSignal;
#endif // _WINDOWS
};
