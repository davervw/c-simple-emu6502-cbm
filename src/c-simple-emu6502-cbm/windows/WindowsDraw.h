// WindowsDraw.h - UI handling for Windows
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

#include "framework.h"
#include <d2d1.h>
#pragma comment(lib, "d2d1.lib")

class WindowsDraw {
public:
	static bool Init(HWND hWnd);
	static bool ReRenderTarget();
	static bool CreateRenderTarget(int screenwidth, int screenheight, int borderwidth, int borderheight, bool& redrawRequiredSignal);
	static void RenderPaint();
	static void FailBox(const char* message);
	static void BeginDraw();
	static void EndDraw();

	static void DrawBorder(byte red, byte green, byte blue);
	static void ClearScreen(byte red, byte green, byte blue);
	static void DrawCharacter2Color(const byte* image, int x, int y, byte fg_red, byte fg_green, byte fg_blue, byte bg_red, byte bg_green, byte bg_blue);

	static ID2D1Factory* factory2d;
	static ID2D1HwndRenderTarget* render2d;
	static RECT clientRect;
	static ID2D1Bitmap* bitmap;
	static float pixelWidth;
	static float pixelHeight;
	static int screenWidth;
	static int screenHeight;
	static int borderWidth;
	static int borderHeight;
	static bool *redrawRequiredSignal;
	static HWND hWnd;
};
