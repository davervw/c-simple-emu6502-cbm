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
#ifndef _WINDOWS
#include "config.h"
#include "cardkbdscan.h"
#ifdef ARDUINO_TEENSY41
#include "USBtoCBMkeyboard.h"
extern USBtoCBMkeyboard usbkbd;
#else
#include "ble_keyboard.h"
#endif // ARUDINO_TEENSY
#endif // NOT _WINDOWS

typedef enum _ShiftState {
	None = 0,
	Shift = 1,
	Control = 2,
	Commodore = 4,
} ShiftState;

static const int buffer_count = 10;
static char buffer[buffer_count];
static int buffer_head = 0;
static int buffer_tail = 0;

static const int scan_codes_count = 16;
static int scan_codes[scan_codes_count];
static int last_scan_code;
static ShiftState shiftState = ShiftState::None;

static const char scan_to_ascii[4][88] = {
	{
		127, '\r', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',
		'3', 'w', 'a', '4', 'z', 's', 'e', '\xff',
		'5', 'r', 'd', '6', 'c', 'f', 't', 'x',
		'7', 'y', 'g', '8', 'b', 'h', 'u', 'v',
		'9', 'i', 'j', '0', 'm', 'k', 'o', 'n',
		'+', 'p', 'l', '-', '.', ':', '@', ',',
		'\\', '*', ';', '\xff', '\xff', '=', '^', '/',
		'1', '_', '\xff', '2', ' ', '\xff', 'q', 3,
		'\xff', '8', '5', '\t', '2', '4', '7', '1',
		27, '+', '-', '\n', '\r', '6', '9', '3',
		'\xff', '0', '*', '\xff', '\xff', '\xff', '\xff', 19,
	},
	{
		127, '\r', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',
		'#', 'W', 'A', '$', 'Z', 'S', 'E', '\xff',
		'%', 'R', 'D', '&', 'C', 'F', 'T', 'X',
		'\'', 'Y', 'G', '(', 'B', 'H', 'U', 'V',
		')', 'I', 'J', '(', 'M', 'K', 'O', 'N',
		'+', 'P', 'L', '-', '>', '[', '@', '<',
		'\\', '*', ']', '\xff', '\xff', '=', '^', '?',
		'!', '_', '\xff', '"', ' ', '\xff', 'Q', 3,
		'\xff', '*', '%', '\t', '2', '4', '7', '1',
		27, '+', '-', '\n', '\r', '6', '9', '3',
		'\xff', '0', '*', '\xff', '\xff', '\xff', '\xff', 19,
	},
	{
		127, 13, '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',
		'\xff', 23, 1, '\xff', 26, 19, 5, '\xff',
		'\xff', 18, 4, 30, 3, 6, 20, 24,
		'\xff', 25, 7, '\xff', 2, 8, 21, 22,
		'\xff', 9, 10, '\xff', 13, 11, 15, 14,
		'\xff', 16, 12, 31, '\xff', 27, 0, '\xff',
		28, '\xff', 29, 12, '\xff', '\xff', 30, 127,
		'\xff', 31, '\xff', 0, '\xff', '\xff', 17, '\xff',
		'\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',
		'\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',
		'\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',
	},
	{
		'\xff', 13, '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',
		'\xff', '}', '\xff', '\xff', '\xff', '\xff', '~', '\xff',
		'\xff', '`', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',
		'\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',
		'\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',
		'\xff', '\xff', '\xff', '|', '\xff', '\xff', '\xff', '\xff',
		'\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',
		'\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '{', '\xff',
		'\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',
		'\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',
		'\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',
	}
};

static void waitKeysReleased();

ASCIIKeyboard::ASCIIKeyboard()
{
	buffer_head = 0;
	buffer_tail = 0;
}

ASCIIKeyboard::~ASCIIKeyboard()
{
	waitKeysReleased();
}

bool ASCIIKeyboard::read(char& c)
{
	c = 0;
	if (!readWaiting())
		return false;
	c = buffer[buffer_head++];
	if (buffer_head == buffer_count)
		buffer_head = 0;
  return true;
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
		if (scan_code == SCAN_CODE_COMMODORE)
			shiftState = ShiftState(shiftState | ShiftState::Commodore);
	}
}

static int calculateScanCode()
{
	int counter = 0;
	bool found = false;
	while (!found && counter < 88) {
		for (int i = 0; i < scan_codes_count; ++i) {
			int scan_code = scan_codes[i];
			if (scan_code == counter && scan_code != SCAN_CODE_LSHIFT && scan_code != SCAN_CODE_RSHIFT && scan_code != SCAN_CODE_CTRL && scan_code != SCAN_CODE_COMMODORE)
			{
				found = true;
				break;
			}
		}
		if (!found)
			++counter;
	}
	//if (counter != 88) dprintf("scan code %d\n", counter);
	return counter;
}

static bool caps = false;

void pollKeyboard()
{
#ifdef _WINDOWS
	WindowsKeyboard::get_scan_codes(scan_codes, scan_codes_count);
#else
// TODO: USB for Teensy
  String s;
#ifndef ARDUINO_TEENSY41
  ble_keyboard->ServiceConnection();
  s = ble_keyboard->Read();
  if (s.length() != 0)
    ;
  else 
#endif
  if (CardKbd)
    s = CardKbdScanRead();
#ifndef ARDUINO_SUNTON_8048S070
#ifndef ARDUINO_TEENSY41
#ifndef ARDUINO_LILYGO_T_DISPLAY_S3
  else if (Serial2.available())
    s = Serial2.readString();
#endif
#endif
#endif
  else if (SerialDef.available())
    s = SerialDef.readString();
#ifdef ARDUINO_TEENSY41
  else
    s = usbkbd.Read();
#endif
  if (s.length() == 0)
	  return;
  caps = false;
  unsigned src = 0;
  int dest = 0;
  int scan = 0;
  int len = 0;
  while (src < s.length() && dest < 16) {
	  char c = s.charAt(src++);
	  if (c >= '0' && c <= '9') {
		  scan = scan * 10 + (c - '0');
		  ++len;
	  }
	  else if (len > 0)
	  {
		  if (scan & 128)
			  caps = true;
		  if (scan == 64 || scan > 88)
			  scan = (scan & 0xFF80) | 88;
		  scan_codes[dest++] = scan;
		  scan = 0;
		  len = 0;
	  }
  }
  while (dest < scan_codes_count)
	  scan_codes[dest++] = 88;
#endif
}

static void pushKey(int scan_code)
{
	char ascii = '\xff';
	if (shiftState == ShiftState::None)
		ascii = scan_to_ascii[0][scan_code];
	else if ((shiftState & ShiftState::Shift) && !(shiftState & (ShiftState::Control|ShiftState::Commodore)))
		ascii = scan_to_ascii[1][scan_code];
	else if (shiftState & ShiftState::Control)
		ascii = scan_to_ascii[2][scan_code];
	else if (shiftState & ShiftState::Commodore)
		ascii = scan_to_ascii[3][scan_code];
	if (ascii & 0x80)
		return;
	if ((buffer_tail + 1) % buffer_count == buffer_head)
		return;
	if (caps && ascii >= 'a' && ascii <= 'z')
		ascii = ascii - 'a' + 'A';
	buffer[buffer_tail] = ascii;
	//dprintf("ASCIIKeyboard.pushKey %02X\n", ascii);
	if (++buffer_tail == buffer_count)
		buffer_tail = 0;
}

static void scanCodesToKeyboardBuffer()
{
	pollKeyboard();
	calculateShiftState();
	int scan_code = calculateScanCode();
	if (scan_code != last_scan_code) {
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

static void waitKeysReleased()
{
	int scan_code;
	do {
		pollKeyboard();
		calculateShiftState();
		scan_code = calculateScanCode();
	} while (scan_code != SCAN_CODE_NO_KEY);
}
