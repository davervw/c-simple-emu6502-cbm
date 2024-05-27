// ASCIIKeyboard.cpp - receive keys from a keyboard as ASCII 0..127
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

//#include "dprintf.h"
#include "ASCIIKeyboard.h"
#include "C128ScanCode.h"

typedef enum {
	None = 0,
	Shift = 1,
	Control = 2,
} ShiftState;

static const int buffer_count = 10;
static char buffer[buffer_count];
static int buffer_head = 0;
static int buffer_tail = 0;

static const int scan_codes_count = 16;
static int scan_codes[scan_codes_count];
static int last_scan_code;
static ShiftState shiftState = ShiftState::None;

static const char scan_to_ascii[3][88] = {
	{
		127, '\r', -1, -1, -1, -1, -1, -1,
		'3', 'w', 'a', '4', 'z', 's', 'e', -1,
		'5', 'r', 'd', '6', 'c', 'f', 't', 'x',
		'7', 'y', 'g', '8', 'b', 'h', 'u', 'v',
		'9', 'i', 'j', '0', 'm', 'k', 'o', 'n',
		'+', 'p', 'l', '-', '.', ':', '@', ',',
		'\\', '*', ';', -1, -1, '=', '^', '/',
		'1', '_', -1, '2', ' ', -1, 'Q', 3,
		-1, '8', '5', '\t', '2', '4', '7', '1',
		27, '+', '-', '\n', '\r', '6', '9', '3',
		-1, '0', '*', -1, -1, -1, -1, 19,
	},
	{
		127, '\r', -1, -1, -1, -1, -1, -1,
		'#', 'W', 'A', '$', 'Z', 'S', 'E', -1,
		'%', 'R', 'D', '&', 'C', 'F', 'T', 'X',
		'\'', 'Y', 'G', '(', 'B', 'H', 'U', 'V',
		')', 'I', 'J', '(', 'M', 'K', 'O', 'N',
		'+', 'P', 'L', '-', '>', '[', '@', '<',
		'\\', '*', ']', -1, -1, '=', '^', '?',
		'!', '_', -1, '"', ' ', -1, 'Q', 3,
		-1, '*', '%', '\t', '2', '4', '7', '1',
		27, '+', '-', '\n', '\r', '6', '9', '3',
		-1, '0', '*', -1, -1, -1, -1, 19,
	},
	{
		127, 13, -1, -1, -1, -1, -1, -1,
		-1, 23, 1, -1, 26, 19, 5, -1,
		-1, 18, 4, -1, 3, 6, 20, 24,
		-1, 25, 7, -1, 2, 8, 21, 22,
		-1, 9, 10, -1, 13, 11, 15, 14,
		-1, 16, 12, -1, -1, -1, 0, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, 0, -1, -1, 17, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
	}
};

ASCIIKeyboard::ASCIIKeyboard()
{
	buffer_head = 0;
	buffer_tail = 0;
}

ASCIIKeyboard::~ASCIIKeyboard()
{
}

bool ASCIIKeyboard::read(char& c)
{
	if (!readWaiting())
		return false;
	c = buffer[buffer_head++];
	if (buffer_head == buffer_count)
		buffer_head = 0;
}

static void calculateShiftState()
{
	shiftState = ShiftState::None;
	for (int i = 0; i < scan_codes_count; ++i) {
		int scan_code = scan_codes[i];
		if (scan_code == SCAN_CODE_LSHIFT || scan_code == SCAN_CODE_RSHIFT)
			shiftState = ShiftState(shiftState | ShiftState::Shift);
		if (scan_code == SCAN_CODE_CTRL)
			shiftState = ShiftState(shiftState | ShiftState::Control);
	}
}

static int calculateScanCode()
{
	int counter = 0;
	bool found = false;
	while (!found && counter < 88) {
		for (int i = 0; i < scan_codes_count; ++i) {
			int scan_code = scan_codes[i];
			if (scan_code == counter && scan_code != SCAN_CODE_LSHIFT && scan_code != SCAN_CODE_RSHIFT && scan_code != SCAN_CODE_CTRL)
			{
				found = true;
				break;
			}
		}
		if (!found)
			++counter;
	}
	return counter;
}

static bool keyPressed()
{
	for (int i = 0; i < scan_codes_count; ++i)
		if (scan_codes[i] != SCAN_CODE_NO_KEY)
			return true;
	return false;
}

void pollKeyboard()
{
#ifdef _WINDOWS
	WindowsKeyboard::get_scan_codes(scan_codes, scan_codes_count);
#else
#endif
}

static void pushKey(int scan_code)
{
	char ascii = -1;
	if (shiftState == 0)
		ascii = scan_to_ascii[0][scan_code];
	else if (shiftState & ShiftState::Shift)
		ascii = scan_to_ascii[1][scan_code];
	else if (shiftState & ShiftState::Control)
		ascii = scan_to_ascii[2][scan_code];
	if (ascii < 0)
		return;
	if ((buffer_tail + 1) % buffer_count == buffer_head)
		return;
	buffer[buffer_tail] = ascii;
	if (++buffer_tail == buffer_count)
		buffer_tail = 0;
}

static void scanCodesToKeyboardBuffer()
{
	pollKeyboard();
	calculateShiftState();
	int scan_code = calculateScanCode();
	if (scan_code != last_scan_code) {
		//dprintf("%d %d\n", scan_code, shiftState);
		last_scan_code = scan_code;
		if (scan_code != SCAN_CODE_NO_KEY)
			pushKey(scan_code);
	}
}

bool ASCIIKeyboard::readWaiting()
{
	scanCodesToKeyboardBuffer();
	return (buffer_head != buffer_tail);
}
