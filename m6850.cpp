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

#include "m6850.h"
#include <stdio.h>

M6850::M6850()
{
	status = 0b01000000;
	control = 0b11000000;
	set_receive_data_register_full(); // simulate always have byte to read
}

M6850::~M6850()
{
}

byte M6850::read_data()
{
	// TODO: on async receive (simulated?) set receive_data_register_full, set interrupt if enabled
	clear_irq();
	byte data = getchar();
	// TODO: clear_receive_data_register_full(); on read
	return data;
}

void M6850::write_data(byte value)
{
	if (control & 0x10)
		putchar(value);
	else
		putchar(value & 0x7F);
	clear_irq();
	clear_transmit_data_register_empty();
	// TODO: delay and set interrupt per transmission time
	set_transmit_data_register_empty();
}

byte M6850::read_status()
{
	return status;
}

void M6850::write_control(byte value)
{
	control = value;
}

bool M6850::read_irq()
{
	return (bool)(status & 1);
}

void M6850::clear_irq()
{
	status &= 0xFE;
}

void M6850::set_irq()
{
	status |= 1;
}

void M6850::clear_receive_data_register_full()
{
	status &= 0x7F;
}

void M6850::set_receive_data_register_full()
{
	status |= 0x80;
}

void M6850::clear_transmit_data_register_empty()
{
	status &= 0xBF;
}

void M6850::set_transmit_data_register_empty()
{
	status |= 0x40;
}
