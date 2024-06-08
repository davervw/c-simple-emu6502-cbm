// emumin.cpp - Minimal 6502 platform
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
#include "dprintf.h"

#ifdef _WINDOWS
#include "WindowsFile.h"
#include "WindowsTime.h"
#else // NOT _WINDOWS
#include "config.h"
#include <FS.h>
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
#include "FFat.h"
#else // NOT ARDUINO_LILYGO_T_DISPLAY_S3
#include <SD.h>
#endif // NOT ARDUINO_LILYGO_T_DISPLAY_S3
#endif // NOT _WINDOWS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "emumin.h"

extern int main_go_num;

static const char* getFilename(Terminal* terminal);

EmuMinimum::EmuMinimum(ushort serialaddr) // TODO: prompt for ROM filename from list
	: Emu6502(new MinimumMemory(serialaddr))
{
	//dprintf("RAM=%d ROM=%d\r\n", ((MinimumMemory*)memory)->getramsize(), ((MinimumMemory*)memory)->getromsize());
	//trace = true;
	sixty_hz_irq = false;
	main_go_num = -1;
}

EmuMinimum::~EmuMinimum()
{
	delete memory;
}

bool EmuMinimum::ExecutePatch()
{
	if (main_go_num >= 0)
		quit = true;
	((MinimumMemory*)memory)->CheckPaintFrame(timer_now);
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

MinimumMemory::MinimumMemory(ushort serialaddr)
{
	terminal = new Terminal();
	uart = new MC6850(terminal, terminal);
	const char* filename = getFilename(terminal);
	this->ramsize = ramsize;
	this->romsize = romsize;
	this->serialaddr = serialaddr;
	unsigned maxram = 0x10000;
	ram = new unsigned char[maxram]; // allocate full 64K
	memset(ram, 0, maxram);
	romsize = EmuCBM::File_ReadAllBytes(ram, maxram, filename);
	romaddr = (ushort)(0x10000 - romsize); // rom loads from end of memory, assumes sized correctly
#ifdef WINDOWS
	memmove_s(&ram[romaddr], romsize, ram, romsize);
#else
	memmove(&ram[romaddr], ram, romsize);
#endif
	ramsize = romaddr;
	memset(ram, 0, ramsize);
}

MinimumMemory::~MinimumMemory()
{
	delete[] ram;
	delete terminal;
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

unsigned MinimumMemory::getramsize() const
{
	return ramsize;
}

unsigned MinimumMemory::getromsize() const
{
	return romsize;
}

static bool isBin(const char* name)
{
	if (name == 0)
		return false;
	auto len = strlen(name);
	if (len < 4)
		return false;
	return 
#ifdef _WINDOWS
		_strcmpi(name + len - 4, ".bin") == 0;
#else
		String(name + len - 4).equalsIgnoreCase(String(".bin"));
#endif
}

static const char* getFilename(Terminal* terminal)
{
	const char* chosenFilename = "/roms/minimum/testmin.bin";

	File dir = SD.open("/roms/minimum");
	if (!dir)
		return chosenFilename;
	File entry = dir.openNextFile();
	int i = 0;
	while (entry)
	{
		if (!entry.isDirectory() && isBin(entry.name())) {
			char buffer[80];      
			snprintf(buffer, sizeof(buffer), "%d. %s %d(%X) bytes\r", ++i, entry.name(), entry.size(), entry.size());
			dprintf("%s", buffer);
			terminal->write(buffer);
#ifdef _WINDOWS      
			terminal->CheckPaintFrame(micros());
#endif      
		}
		entry.close();
		entry = dir.openNextFile();
	}
	dir.close();

	if (i == 0)
		return chosenFilename;

	terminal->write("Choice? ");

	int n = 0;
	while (true) {
		char c;
		if (!terminal->read(c)) {
#ifdef _WINDOWS      
			terminal->CheckPaintFrame(micros());
#endif
			continue;
		}
		n = 0;
		if (c >= '1' && c <= '9')
			n = c - '0';
		if (c == '0')
			n = 10;
		if (n >= 1 && n <= i) {
			terminal->write(c);
			terminal->write('\r');
			break;
		}
	}

	const char* dirname = "/roms/minimum";
	dir = SD.open(dirname);
	if (!dir)
		return chosenFilename;
	entry = dir.openNextFile();
	i = 0;
	bool found = false;
	while (entry && !found)
	{
		if (!entry.isDirectory() && isBin(entry.name()) && ++i == n) {
			static char buffer[80];
			snprintf(buffer, sizeof(buffer), "%s/%s", dirname, entry.name());
			chosenFilename = buffer;
			found = true;
		}
		entry.close();
		entry = dir.openNextFile();
	}
	dir.close();

	terminal->write('\r');
	terminal->write(chosenFilename);
	terminal->write('\r');
	return chosenFilename;
}
