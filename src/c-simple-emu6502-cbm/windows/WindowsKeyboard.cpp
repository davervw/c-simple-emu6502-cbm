// WindowsKeyboard.cpp - Windows to C128 scan codes keyboard driver
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
#include "WindowsKeyboard.h"

static int scan_codes[16] = { 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88 };

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
	SCAN_CODE_FLAG_FORCE_NOSHIFT = 4096
} C128ScanCode;

typedef enum {
	NONE = 0,
	ANYSHIFT = 1,
	ANYCTRL = 2,
	ANYALT = 4,
	LSHIFT = 8,
	RSHIFT = 16,
	LCTRL = 32,
	RCTRL = 64,
	LALT = 128,
	RALT = 256,
} WinShiftState;

static WinShiftState shiftState = NONE;

typedef struct {
	WinShiftState state;
	WPARAM vkey;
	C128ScanCode code;
} KeyMap;

static KeyMap WindowsToC128[] = { // TODO: handle SHIFT, etc. cases for key combinations
	{ NONE, VK_RETURN, SCAN_CODE_RETURN },
	{ NONE, VK_BACK, SCAN_CODE_BACKSPACE },
	{ NONE, VK_DELETE, SCAN_CODE_BACKSPACE },
	{ NONE, VK_INSERT, (C128ScanCode)(SCAN_CODE_BACKSPACE | SCAN_CODE_FLAG_FORCE_SHIFT) },
	{ NONE, VK_RIGHT, SCAN_CODE_RIGHT },
	{ NONE, VK_F7, SCAN_CODE_F7 },
	{ NONE, VK_F8, (C128ScanCode)(SCAN_CODE_F7 | SCAN_CODE_FLAG_FORCE_SHIFT) },
	{ NONE, VK_F1, SCAN_CODE_F1 },
	{ NONE, VK_F2, (C128ScanCode)(SCAN_CODE_F1 | SCAN_CODE_FLAG_FORCE_SHIFT) },
	{ NONE, VK_F3, SCAN_CODE_F3 },
	{ NONE, VK_F4, (C128ScanCode)(SCAN_CODE_F3 | SCAN_CODE_FLAG_FORCE_SHIFT) },
	{ NONE, VK_F5, SCAN_CODE_F5 },
	{ NONE, VK_F6, (C128ScanCode)(SCAN_CODE_F5 | SCAN_CODE_FLAG_FORCE_SHIFT) },
	{ NONE, VK_DOWN, SCAN_CODE_DOWN },
	{ NONE, '3', SCAN_CODE_3 },
	{ NONE, 'W', SCAN_CODE_W},
	{ NONE, 'A', SCAN_CODE_A },
	{ NONE, '4', SCAN_CODE_4},
	{ NONE, 'Z', SCAN_CODE_Z},
	{ NONE, 'S', SCAN_CODE_S},
	{ NONE, 'E', SCAN_CODE_E},
	{ NONE, VK_LSHIFT, SCAN_CODE_LSHIFT },
	{ NONE, '5', SCAN_CODE_5 },
	{ NONE, 'R', SCAN_CODE_R },
	{ NONE, 'D', SCAN_CODE_D },
	{ ANYSHIFT, '6', (C128ScanCode)(SCAN_CODE_UPARROW | SCAN_CODE_FLAG_FORCE_NOSHIFT) },
	{ NONE, '6', SCAN_CODE_6 },
	{ NONE, 'C', SCAN_CODE_C },
	{ NONE, 'F', SCAN_CODE_F },
	{ NONE, 'T', SCAN_CODE_T },
	{ NONE, 'X', SCAN_CODE_X },
	{ ANYSHIFT, '7', SCAN_CODE_6 },
	{ NONE, '7', SCAN_CODE_7 },
	{ ANYSHIFT, VK_OEM_7, SCAN_CODE_2 },
	{ NONE, VK_OEM_7, (C128ScanCode)(SCAN_CODE_7 | SCAN_CODE_FLAG_FORCE_SHIFT) },
	{ NONE, 'Y', SCAN_CODE_Y },
	{ NONE, 'G', SCAN_CODE_G },
	{ ANYSHIFT, '8', (C128ScanCode)(SCAN_CODE_ASTERISK | SCAN_CODE_FLAG_FORCE_NOSHIFT) },
	{ NONE, '8', SCAN_CODE_8 },
	{ NONE, 'B', SCAN_CODE_B },
	{ NONE, 'H', SCAN_CODE_H },
	{ NONE, 'U', SCAN_CODE_U },
	{ NONE, 'V', SCAN_CODE_V },
	{ ANYSHIFT, '9', SCAN_CODE_8 },
	{ NONE, '9', SCAN_CODE_9 },
	{ NONE, 'I', SCAN_CODE_I },
	{ NONE, 'J', SCAN_CODE_J },
	{ ANYSHIFT, '0', SCAN_CODE_9 },
	{ NONE, '0', SCAN_CODE_0 },
	{ NONE, 'M', SCAN_CODE_M },
	{ NONE, 'K', SCAN_CODE_K },
	{ NONE, 'O', SCAN_CODE_O },
	{ NONE, 'N', SCAN_CODE_N },
	{ ANYSHIFT, '+' + 0x90, C128ScanCode(SCAN_CODE_PLUS | SCAN_CODE_FLAG_FORCE_NOSHIFT) },
	{ NONE, '+' + 0x90, SCAN_CODE_EQUALS },
	{ NONE, 'P', SCAN_CODE_P },
	{ NONE, 'L', SCAN_CODE_L },
	{ ANYSHIFT, VK_OEM_MINUS, C128ScanCode(SCAN_CODE_LTARROW | SCAN_CODE_FLAG_FORCE_SHIFT) },
	{ NONE, VK_OEM_MINUS, SCAN_CODE_MINUS },
	{ NONE, '.' + 0x90, SCAN_CODE_PERIOD},
	//{ NONE, VK_OEM_1, SCAN_CODE_COLON },
	{ NONE, VK_OEM_4 , C128ScanCode(SCAN_CODE_COLON | SCAN_CODE_FLAG_FORCE_SHIFT) },
	//{ NONE, VK_NUMPAD2, SCAN_CODE_AT },
	{ NONE, ',' + 0x90, SCAN_CODE_COMMA },
	{ NONE, VK_OEM_5, SCAN_CODE_POUND },
	//{ NONE, '*', SCAN_CODE_ASTERISK },
	{ ANYSHIFT, VK_OEM_1, C128ScanCode(SCAN_CODE_COLON | SCAN_CODE_FLAG_FORCE_NOSHIFT) },
	{ NONE, VK_OEM_1, SCAN_CODE_SEMICOLON },
	{ NONE, VK_OEM_6, C128ScanCode(SCAN_CODE_SEMICOLON | SCAN_CODE_FLAG_FORCE_SHIFT) },
	{ NONE, VK_HOME, SCAN_CODE_HOME },
	{ NONE, VK_RSHIFT, SCAN_CODE_RSHIFT },
	{ NONE, '=' + 0x90, SCAN_CODE_EQUALS},
	//{ NONE, '^' + 0x90, SCAN_CODE_UPARROW},
	{ NONE, '/' + 0x90, SCAN_CODE_SLASH },
	{ NONE, '1', SCAN_CODE_1},
	//{ NONE, '_' + 0x90, SCAN_CODE_LTARROW},
	{ NONE, VK_TAB, SCAN_CODE_CTRL },
	{ ANYSHIFT, '2', C128ScanCode(SCAN_CODE_AT | SCAN_CODE_FLAG_FORCE_NOSHIFT) },
	{ NONE, '2', SCAN_CODE_2 },
	{ NONE, ' ', SCAN_CODE_SPACE},
	{ NONE, VK_CONTROL, SCAN_CODE_COMMODORE },
	{ NONE, 'Q', SCAN_CODE_Q},
	{ NONE, VK_CANCEL, SCAN_CODE_STOP },
	{ NONE, VK_ESCAPE, SCAN_CODE_STOP },
	//{ NONE, VK_HELP, SCAN_CODE_HELP },
	{ NONE, VK_NUMPAD8, SCAN_CODE_NUM_8 },
	{ NONE, VK_NUMPAD5, SCAN_CODE_NUM_5 },
	{ NONE, VK_TAB, SCAN_CODE_TAB },
	{ NONE, VK_NUMPAD2, SCAN_CODE_NUM_2 },
	{ NONE, VK_NUMPAD4, SCAN_CODE_NUM_4 },
	{ NONE, VK_NUMPAD7, SCAN_CODE_NUM_7 },
	{ NONE, VK_NUMPAD1, SCAN_CODE_NUM_1 },
	//{ NONE, VK_ESCAPE, SCAN_CODE_ESC },
	{ NONE, VK_ADD, SCAN_CODE_NUM_PLUS},
	{ NONE, VK_SUBTRACT, SCAN_CODE_NUM_MINUS},
	{ NONE, VK_DIVIDE, SCAN_CODE_SLASH },
	{ NONE, VK_MULTIPLY, SCAN_CODE_ASTERISK },
	//{ NONE, , SCAN_CODE_LINE_FEED },
	//{ NONE, '\r', SCAN_CODE_ENTER},
	{ NONE, VK_NUMPAD6, SCAN_CODE_NUM_6 },
	{ NONE, VK_NUMPAD9, SCAN_CODE_NUM_9 },
	{ NONE, VK_NUMPAD3, SCAN_CODE_NUM_3 },
	{ NONE, VK_LMENU, SCAN_CODE_ALT },
	{ NONE, VK_RMENU, SCAN_CODE_ALT },
	{ NONE, VK_NUMPAD0, SCAN_CODE_NUM_0 },
	{ NONE, VK_DECIMAL, SCAN_CODE_NUM_DECIMAL },
	{ NONE, VK_UP, (C128ScanCode)(SCAN_CODE_CURSOR_UP | SCAN_CODE_FLAG_FORCE_SHIFT) },
	{ NONE, VK_DOWN, SCAN_CODE_CURSOR_DOWN },
	{ NONE, VK_LEFT, (C128ScanCode)(SCAN_CODE_CURSOR_LEFT | SCAN_CODE_FLAG_FORCE_SHIFT) },
	{ NONE, VK_RIGHT, SCAN_CODE_CURSOR_RIGHT },
	{ NONE, VK_SCROLL, SCAN_CODE_NO_SCROLL },
	{ NONE, VK_PAUSE, SCAN_CODE_NO_SCROLL },
	{ NONE, VK_PRIOR, (C128ScanCode)(SCAN_CODE_NO_KEY | SCAN_CODE_FLAG_RESTORE) }
};

static C128ScanCode find_scan_code_by_windows_key(WPARAM key, LPARAM lParam)
{
	const int limit = sizeof(WindowsToC128) / sizeof(*WindowsToC128);
	for (int i = 0; i < limit; ++i) {
		if ((WindowsToC128[i].state & shiftState) == WindowsToC128[i].state && WindowsToC128[i].vkey == key) {
			return WindowsToC128[i].code;
		}
	}
	return SCAN_CODE_NO_KEY;
}

void static append_scan_code(C128ScanCode code)
{
	static const int limit = sizeof(scan_codes) / sizeof(*scan_codes) - 1; // reserve one for shift

	bool forceShift = ((int)code & (int)C128ScanCode::SCAN_CODE_FLAG_FORCE_SHIFT) != 0;
	if (forceShift)
		code = (C128ScanCode)(code ^ (int)C128ScanCode::SCAN_CODE_FLAG_FORCE_SHIFT);
	bool forceUnshift = ((int)code & (int)C128ScanCode::SCAN_CODE_FLAG_FORCE_NOSHIFT) != 0;
	if (forceUnshift)
		code = (C128ScanCode)(code ^ (int)C128ScanCode::SCAN_CODE_FLAG_FORCE_NOSHIFT);

	int i = 0;
	while (i < limit && scan_codes[i] != code)
		++i;
	if (i < limit)
		return; // already there
	i = 0;
	while (i < limit && scan_codes[i] != SCAN_CODE_NO_KEY)
		++i;
	if (i < limit && scan_codes[i] == SCAN_CODE_NO_KEY) {
		scan_codes[i] = code;
		//{wchar_t buffer[80]{};_snwprintf_s(buffer, sizeof(buffer), _T("scan code %04X\n"), code);OutputDebugStringW(buffer);}
		if (forceShift)
			scan_codes[limit] = SCAN_CODE_LSHIFT;
		if (forceUnshift && (scan_codes[0] == SCAN_CODE_LSHIFT || scan_codes[0] == SCAN_CODE_RSHIFT))
			scan_codes[0] = SCAN_CODE_NO_KEY;
	}
}

void static release_scan_code(C128ScanCode code)
{
	static const int limit = sizeof(scan_codes) / sizeof(*scan_codes) - 1;

	bool forceShift = ((int)code & (int)SCAN_CODE_FLAG_FORCE_SHIFT) != 0;
	if (forceShift)
		code = (C128ScanCode)(code ^ (int)SCAN_CODE_FLAG_FORCE_SHIFT);
	bool forceUnshift = ((int)code & (int)C128ScanCode::SCAN_CODE_FLAG_FORCE_NOSHIFT) != 0;
	if (forceUnshift)
		code = (C128ScanCode)(code ^ (int)C128ScanCode::SCAN_CODE_FLAG_FORCE_NOSHIFT);

	int i = 0;
	while (i < limit && scan_codes[i] != code)
		++i;
	if (i < limit) {
		scan_codes[i] = SCAN_CODE_NO_KEY;
		if (forceShift)
			scan_codes[limit] = SCAN_CODE_NO_KEY;
		if (forceUnshift && (shiftState & ANYSHIFT)) // push shift back into buffer
		{
			for (i = 0; i < limit; ++i) {
				if (scan_codes[i] == SCAN_CODE_NO_KEY)
				{
					scan_codes[i] = (shiftState & LSHIFT) ? SCAN_CODE_LSHIFT : SCAN_CODE_RSHIFT;
					break;
				}
			}
		}
	}
}

LRESULT WindowsKeyboard::ReceiveMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	const wchar_t* szMessage;
	switch (message)
	{
	case WM_KEYDOWN:
	{
		if (lParam & 0x40000000)
			return 0; // already down, repeating
		szMessage = _T("key down");

		unsigned char windows_scancode = (unsigned char)(lParam >> 16);
		bool isExtended = (lParam >> 17) & 1;
		if (wParam == VK_SHIFT)
			wParam = MapVirtualKey(windows_scancode, MAPVK_VSC_TO_VK_EX);

		C128ScanCode code = find_scan_code_by_windows_key(wParam, lParam);
		if (code != SCAN_CODE_NO_KEY)
			append_scan_code(code);

		if (wParam == VK_LSHIFT)
			shiftState = WinShiftState(shiftState | LSHIFT | ANYSHIFT);
		else if (wParam == VK_RSHIFT)
			shiftState = WinShiftState(shiftState | RSHIFT | ANYSHIFT);

		break;
	}
	case WM_KEYUP:
	{
		szMessage = _T("key up");

		unsigned char windows_scancode = (unsigned char)(lParam >> 16);
		bool isExtended = (lParam >> 17) & 1;
		if (wParam == VK_SHIFT)
			wParam = MapVirtualKey(windows_scancode, MAPVK_VSC_TO_VK_EX);

		C128ScanCode code = find_scan_code_by_windows_key(wParam, lParam);
		if (code != SCAN_CODE_NO_KEY)
			release_scan_code(code);

		if (wParam == VK_LSHIFT || wParam == VK_RSHIFT)
			shiftState = WinShiftState((int)shiftState & ~(LSHIFT | RSHIFT | ANYSHIFT));

		break;
	}
	case WM_SYSKEYDOWN:
		szMessage = _T("system key down");
		break;
	case WM_SYSKEYUP:
		szMessage = _T("system key up");
		break;
	default:
		szMessage = _T("???");
		break;
	}
	//{wchar_t buffer[80]{};_snwprintf_s(buffer, sizeof(buffer), _T("%s %llX %llX %d\n"), szMessage, (long long)wParam, (long long)lParam, shiftState);OutputDebugStringW(buffer);}
	return 0;
}

void WindowsKeyboard::get_scan_codes(int* scan_codes, int count)
{
	static const int limit = sizeof(::scan_codes) / sizeof(*::scan_codes);
	int x = limit;
	for (int i = 0; i < count; ++i)
		scan_codes[i] = (i < limit) ? ::scan_codes[i] : SCAN_CODE_NO_KEY;
	count = limit;
}
#endif // _WINDOWS