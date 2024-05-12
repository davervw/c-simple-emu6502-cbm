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

extern int main_go_num;

EmuMinimum::EmuMinimum(const char* filename, ushort serialaddr, bool line_editor)
	: Emu6502(new MinimumMemory(filename, serialaddr, line_editor))
{
	printf("RAM=%d ROM=%d\n", ((MinimumMemory*)memory)->getramsize(), ((MinimumMemory*)memory)->getromsize());
}

EmuMinimum::~EmuMinimum()
{
}

bool EmuMinimum::ExecutePatch()
{
	if (main_go_num != 1)
		quit = true;
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

MinimumMemory::MinimumMemory(const char* filename, ushort serialaddr, bool line_editor)
{
	uart = new MC6850(line_editor);
	this->ramsize = ramsize;
	this->romsize = romsize;
	this->serialaddr = serialaddr;
	unsigned maxram = 0x10000;
	ram = new unsigned char[maxram]; // allocate full 64K
	memset(ram, 0, maxram);
	romsize = EmuCBM::File_ReadAllBytes(ram, maxram, filename);
	romaddr = (ushort)(0x10000 - romsize); // rom loads from end of memory, assumes sized correctly
	memmove_s(&ram[romaddr], romsize, ram, romsize);
	ramsize = romaddr;
	memset(ram, 0, ramsize);
}

MinimumMemory::~MinimumMemory()
{
	delete[] ram;
}

byte MinimumMemory::read(ushort addr)
{
	if (addr == serialaddr)
		return uart->read_data();
	if (addr == serialaddr + 1)
		return uart->read_status();
	return ram[addr];
}

void MinimumMemory::write(ushort addr, byte value)
{
	if (addr == serialaddr)
		uart->write_data(value);
	else if (addr == serialaddr + 1)
		uart->write_control(value);
	else if (addr < ramsize)
		ram[addr] = value;
	else if (addr == 0xFFFF)
		main_go_num = value;
}

unsigned MinimumMemory::getramsize()
{
	return ramsize;
}

unsigned MinimumMemory::getromsize()
{
	return romsize;
}
