// LCDDraw.h - UI handling for embedded LCDs
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

// Current resolutions supported:
//                                         vdc/ term  vic ii         vic
//                                         640  200  320  200     176   184
// ARDUINO_SUNTON_8048S070      800x480     1x   2x   2x  2x       4x     2x
// ARDUINO_LILYGO_T_DISPLAY_S3	320x170    .5x .75x   1x .75x   1.75x 0.875x
// M5STACK                      320x240    .5x   1x   1x    1x  1.75x     1x
// ILI9341                      320x240R   .5x   1x   1x    1x  1.75x     1x
// ILI9488                      480x320R 0.75x 1.5x  1.5x 1.5x   2.5x   1.5x

/// scale of 8 pixels
//   .5x = 4   color average adjacent pixels
//  .75x = 6   color average top 2, bottom 2 rows
// .875x = 7   color average bottom 2 rows
//    1x = 8   direct mapping
//  1.5x = 12  every other line %2==1 average adjacent pixels, draw another
// 1.75x = 14  draw each column, and an extra average except fourth column
//    2x = 16  draw two pixels instead of one
//  2.5x = 20  double columns, and on %2==1 average adjacent pixels, draw another
//    4x = 32  draw four pixels instead of one

// these methods are employed to keep the image as crisp as possible
//   in contrast with always color averaging
// the trickier ones are when color averaging across and down at the same time

#ifndef _WINDOWS
class LCDDraw // TODO: implement for LCD screens, more resolution independence
{
public:
	static bool CreateRenderTarget(int screenwidth, int screenheight, int scalex, int scaley);
	static void DrawCharacter2Color(byte* src, int col, int row, int fg, int bg);
	static void ClearAll(int color);
	static void ClearScreenArea(int color);
	static void DrawBorder(int color);
private:
	// native resolution (e.g. 320x240)
	static int clientwidth;
	static int clientheight;

	// emulated system resolution (e.g. 320x200)
	static int screenwidth;
	static int screenheight;
	static int borderwidth;
	static int borderheight;

	// how many native pixels to use for emulated 8 pixels
	static int scalex; 
	static int scaley;
};
#endif
