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
#include <conio.h>

MC6850::MC6850(bool line_editor)
{
	this->line_editor = line_editor;

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
	{
		clear_receive_data_register_full();
		set_receive_data_register_full(); // TODO: wait for data
	}
	return data;
}

void MC6850::write_data(byte value)
{
	// TODO: parity
	if (control.mode == MODE::b8e1
		|| control.mode == MODE::b8n1
		|| control.mode == MODE::b8n2
		|| control.mode == MODE::b8o1
	)
		putchar(value);
	else
		putchar(value & 0x7F);
	clear_irq();
	clear_transmit_data_register_empty();
	// TODO: delay and set interrupt per transmission time
	set_transmit_data_register_empty();
}

byte MC6850::read_status()
{
	status.rdrf = _kbhit();
	return status.value;
}

void MC6850::write_control(byte value)
{
	CONTROL incoming { };
	incoming.value = value;
	if (control.clk == CLOCK::RESET && incoming.clk != CLOCK::RESET) {
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
