// minimum.cpp - Minimal 6502 platform
//
// variable RAM, variable ROM, memory mapped Motorola 8250 compatible UART
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

#include "emucbm.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "emumin.h"

EmuMinimum::EmuMinimum(const char* filename, ushort ramsize, ushort romsize, ushort serialaddr)
	: Emu6502(new MinimumMemory(filename, ramsize, romsize, serialaddr))
{
	step = true;
}

EmuMinimum::~EmuMinimum()
{
}

bool EmuMinimum::ExecutePatch()
{
	return false;
}

byte EmuMinimum::GetMemory(ushort addr)
{
	return memory->read(addr);
}

void EmuMinimum::SetMemory(ushort addr, byte value)
{
	memory->write(addr, value);
}

MinimumMemory::MinimumMemory(const char* filename, ushort ramsize, ushort romsize, ushort serialaddr)
{
	this->ramsize = ramsize;
	this->romsize = romsize;
	this->serialaddr = serialaddr;
	ram = new unsigned char[0x100000]; // allocate full 64K
	memset(ram, 0, ramsize);
	romaddr = (ushort)(0x10000 - romsize); // rom loads from end of memory, assumes sized correctly
	EmuCBM::File_ReadAllBytes(&ram[romaddr], romsize, filename);
}

MinimumMemory::~MinimumMemory()
{
	delete[] ram;
}

byte MinimumMemory::read(ushort addr)
{
	if (addr == serialaddr)
		return uart.read_data();
	if (addr == serialaddr + 1)
		return uart.read_status();
	return ram[addr];
}

void MinimumMemory::write(ushort addr, byte value)
{
	if (addr == serialaddr)
		uart.write_data(value);
	else if (addr == serialaddr + 1)
		uart.write_control(value);
	else if (addr < ramsize)
		ram[addr] = value;
}
