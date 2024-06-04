// C128ScanCode.h - Commodore 128 keyboard scan codes for keyboard drivers
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

typedef enum {
	SCAN_CODE_BACKSPACE = 0,
	SCAN_CODE_RETURN = 1,
	SCAN_CODE_RIGHT = 2,
	SCAN_CODE_F7 = 3,
	SCAN_CODE_F1 = 4,
	SCAN_CODE_F3 = 5,
	SCAN_CODE_F5 = 6,
	SCAN_CODE_DOWN = 7,
	SCAN_CODE_3 = 8,
	SCAN_CODE_W = 9,
	SCAN_CODE_A = 10,
	SCAN_CODE_4 = 11,
	SCAN_CODE_Z = 12,
	SCAN_CODE_S = 13,
	SCAN_CODE_E = 14,
	SCAN_CODE_LSHIFT = 15,
	SCAN_CODE_5 = 16,
	SCAN_CODE_R = 17,
	SCAN_CODE_D = 18,
	SCAN_CODE_6 = 19,
	SCAN_CODE_C = 20,
	SCAN_CODE_F = 21,
	SCAN_CODE_T = 22,
	SCAN_CODE_X = 23,
	SCAN_CODE_7 = 24,
	SCAN_CODE_Y = 25,
	SCAN_CODE_G = 26,
	SCAN_CODE_8 = 27,
	SCAN_CODE_B = 28,
	SCAN_CODE_H = 29,
	SCAN_CODE_U = 30,
	SCAN_CODE_V = 31,
	SCAN_CODE_9 = 32,
	SCAN_CODE_I = 33,
	SCAN_CODE_J = 34,
	SCAN_CODE_0 = 35,
	SCAN_CODE_M = 36,
	SCAN_CODE_K = 37,
	SCAN_CODE_O = 38,
	SCAN_CODE_N = 39,
	SCAN_CODE_PLUS = 40,
	SCAN_CODE_P = 41,
	SCAN_CODE_L = 42,
	SCAN_CODE_MINUS = 43,
	SCAN_CODE_PERIOD = 44,
	SCAN_CODE_COLON = 45,
	SCAN_CODE_AT = 46,
	SCAN_CODE_COMMA = 47,
	SCAN_CODE_POUND = 48,
	SCAN_CODE_ASTERISK = 49,
	SCAN_CODE_SEMICOLON = 50,
	SCAN_CODE_HOME = 51,
	SCAN_CODE_RSHIFT = 52,
	SCAN_CODE_EQUALS = 53,
	SCAN_CODE_UPARROW = 54,
	SCAN_CODE_SLASH = 55,
	SCAN_CODE_1 = 56,
	SCAN_CODE_LTARROW = 57,
	SCAN_CODE_CTRL = 58,
	SCAN_CODE_2 = 59,
	SCAN_CODE_SPACE = 60,
	SCAN_CODE_COMMODORE = 61,
	SCAN_CODE_Q = 62,
	SCAN_CODE_STOP = 63,
	SCAN_CODE_HELP = 64,
	SCAN_CODE_NUM_8 = 65,
	SCAN_CODE_NUM_5 = 66,
	SCAN_CODE_TAB = 67,
	SCAN_CODE_NUM_2 = 68,
	SCAN_CODE_NUM_4 = 69,
	SCAN_CODE_NUM_7 = 70,
	SCAN_CODE_NUM_1 = 71,
	SCAN_CODE_ESC = 72,
	SCAN_CODE_NUM_PLUS = 73,
	SCAN_CODE_NUM_MINUS = 74,
	SCAN_CODE_LINE_FEED = 75,
	SCAN_CODE_ENTER = 76,
	SCAN_CODE_NUM_6 = 77,
	SCAN_CODE_NUM_9 = 78,
	SCAN_CODE_NUM_3 = 79,
	SCAN_CODE_ALT = 80,
	SCAN_CODE_NUM_0 = 81,
	SCAN_CODE_NUM_DECIMAL = 82,
	SCAN_CODE_CURSOR_UP = 83,
	SCAN_CODE_CURSOR_DOWN = 84,
	SCAN_CODE_CURSOR_LEFT = 85,
	SCAN_CODE_CURSOR_RIGHT = 86,
	SCAN_CODE_NO_SCROLL = 87,
	SCAN_CODE_NO_KEY = 88,
	SCAN_CODE_FLAG_RESTORE = 1024,
	SCAN_CODE_FLAG_FORCE_SHIFT = 2048,
	SCAN_CODE_FLAG_FORCE_NOSHIFT = 4096,
	SCAN_CODE_FLAG_FORCE_COMMODORE = 8192,
} C128ScanCode;
