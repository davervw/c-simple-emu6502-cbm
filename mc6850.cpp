// m6850.cpp - Motorola 6850 UART simulation via console terminal
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C++ Portable Version)
// C64/6502 Emulator for Microsoft Windows Console
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

#include "mc6850.h"
#include <stdio.h>
#ifdef WINDOWS
#include <conio.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

static byte readbyte;
static byte readbyte_count = 0;

static bool _kbhit()
{
	if (readbyte_count > 0)
		return true;

	int result = read(STDIN_FILENO, &readbyte, 1);
	if (result >= 0) 
		readbyte_count = result;
	return (bool)readbyte_count;
}

static byte _getch()
{
	if (readbyte_count == 0 && !_kbhit())
		return 0;

	readbyte_count = 0; // mark read
	return readbyte;
}
struct termios save_term;
#endif

MC6850::MC6850(bool line_editor)
{
	this->line_editor = line_editor;

#ifndef WINDOWS
	if (!line_editor)
	{
		struct termios term;
		tcgetattr(STDIN_FILENO, &save_term);
		cfmakeraw(&term);
		term.c_cc[VMIN] = 0;
		term.c_cc[VTIME] = 0;
		tcsetattr(STDIN_FILENO, TCSANOW, &term);
	}
#endif

	status.value = 0;

	control.value = 0;
	control.rts_tint = RTS_TINT::low_disabled;
	control.clk = CLOCK::RESET;

	// require reset by user before use
	clear_receive_data_register_full();
	clear_transmit_data_register_empty();
}

MC6850::~MC6850()
{
#ifndef WINDOWS
	if (!line_editor)
		tcsetattr(STDIN_FILENO, TCSANOW, &save_term);
#endif
}

byte MC6850::read_data()
{
	// TODO: on async receive (simulated?) set receive_data_register_full, set interrupt if enabled
	clear_irq();
	byte data = 0;
	bool keypressed = false;
	if (line_editor)
	{
		data = getchar();
		keypressed = true;
	}
	else if (_kbhit())
	{
		data = _getch();
		keypressed = true;
	}
	if (control.mode == MODE::b7e1
		|| control.mode == MODE::b7e2
		|| control.mode == MODE::b7o1
		|| control.mode == MODE::b7o2
		)
		data = data & 0x7F; // TODO: parity
	if (keypressed)
		clear_receive_data_register_full();

	// standardize backspace so firmware can be consistent
	if (data == 0x7F)
		data = 8;

	return data;
}

void MC6850::write_data(byte value)
{
	if (control.clk == CLOCK::RESET)
		return; // require configuration

	// TODO: parity
	if (control.mode == MODE::b7e1
		|| control.mode == MODE::b7e2
		|| control.mode == MODE::b7o1
		|| control.mode == MODE::b7o2)
	{
		value &= 0x7F;
	}

	if (value == '\n')
		putchar('\r'); // insert carriage return before newline
	if (value >= ' ' || value == '\r' || value == '\n' || value == 8 || value == 7) // skip most other control codes
		putchar(value);
	if (value == '\r')
		putchar('\n'); // add newline after carriage return

	fflush(stdout);
	clear_irq();
	clear_transmit_data_register_empty();
	// TODO: delay and set interrupt per transmission time
	set_transmit_data_register_empty();
}

byte MC6850::read_status()
{
	if (!line_editor)
		status.rdrf = _kbhit() ? 1 : 0;
	return status.value;
}

void MC6850::write_control(byte value)
{
	CONTROL incoming { };
	incoming.value = value;
	if (control.clk == CLOCK::RESET && incoming.clk != CLOCK::RESET) {
		if (line_editor)
			set_receive_data_register_full(); // simulate always have byte to read // TODO: check
		set_transmit_data_register_empty();
	}
	else if (incoming.clk == CLOCK::RESET) {
		status.value = 0;
		status.tdre = 1;
	}
	control.value = value;
}

bool MC6850::read_irq() const
{
	return (bool)(status.irqn);
}

void MC6850::clear_irq()
{
	status.irqn = 0;
}

void MC6850::set_irq()
{
	status.irqn = 1;
}

void MC6850::clear_receive_data_register_full()
{
	status.rdrf = 0;
}

void MC6850::set_receive_data_register_full()
{
	status.rdrf = 1;
}

void MC6850::clear_transmit_data_register_empty()
{
	status.tdre = 0;
}

void MC6850::set_transmit_data_register_empty()
{
	status.tdre = 1;
}
