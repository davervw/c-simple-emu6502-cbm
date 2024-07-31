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
//#include "dprintf.h"
#include "CBMkeyboard.h"

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

static char* getFilename(Terminal* terminal);

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
    Terminal* terminal = ((MinimumMemory*)memory)->terminal;
    if (terminal->specialKey != 0) {
        ProcessSpecialKey(terminal->specialKey);
        terminal->specialKey = 0;
    }
    if (CBMkeyboard::heldToggle)
        main_go_num = 64;
    return false;
}

byte EmuMinimum::GetMemory(ushort addr)
{
    return memory->read(addr);
}

void EmuMinimum::SetMemory(ushort addr, byte value)
{
    memory->write(addr, value);
    if (addr == 0xFFFE)
        trace = (bool)(value >> 7);
}

MinimumMemory::MinimumMemory(ushort serialaddr)
{
    terminal = new Terminal();
    uart = new MC6850(terminal, terminal);
    this->serialaddr = serialaddr;
    unsigned maxram = 0x10000;
    ram = new unsigned char[maxram]; // allocate full 64K
    memset(ram, 0, maxram);
    char* filename = getFilename(terminal);
    romsize = EmuCBM::File_ReadAllBytes(ram, maxram, filename);
    filename = 0;
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
#ifdef _WINDOWS
    if (_strcmpi(name, "asciifont.bin") == 0)
#else
    if (String(name).equalsIgnoreCase("asciifont.bin"))
#endif
        return false;
    if (len < 4)
        return false;
#ifdef _WINDOWS
    return _strcmpi(name + len - 4, ".bin") == 0;
#else
    return String(name + len - 4).equalsIgnoreCase(String(".bin"));
#endif
}

static char* getFilename(Terminal* terminal)
{
    static char chosenFilename[80];
    snprintf(chosenFilename, sizeof(chosenFilename), "%s", "/roms/minimum/testmin.bin");
    char* dirFilenames[10]{};
    int i;

    for (i = 0; i < 10; ++i)
        dirFilenames[i] = 0;
    const char* directoryPath = "/roms/minimum";
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    fs::File dir = FFat.open(directoryPath);
#else
    File dir = SD.open(directoryPath);
#endif    
    if (!dir)
        return chosenFilename;
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    fs::File entry = dir.openNextFile();
#else
    File entry = dir.openNextFile();
#endif
    i = 0;
    while (entry && i < 9)
    {
        if (!entry.isDirectory() && isBin(entry.name())) {
            char buffer[80];
            snprintf(buffer, sizeof(buffer), "%d. %s %d(%X) bytes\r", ++i, entry.name(), (int)entry.size(), (int)entry.size());
            terminal->write(buffer);

            snprintf(buffer, sizeof(buffer), "%s/%s", directoryPath, entry.name());
#ifdef _WINDOWS      
            dirFilenames[i] = _strdup(buffer);
            terminal->CheckPaintFrame(micros());
#else
            dirFilenames[i] = strdup(buffer);
#endif      
        }
        entry.close();
        entry = dir.openNextFile();
    }
    dir.close();

    bool foundNone = (i == 0);
    if (foundNone)
        return chosenFilename; // note: probably won't work if didn't find any filenames

    bool foundOnlyOne = (i == 1);
    if (foundOnlyOne) {
        snprintf(chosenFilename, sizeof(chosenFilename), "%s", dirFilenames[1]);
        free(dirFilenames[1]);
        terminal->clearScreen();
        return chosenFilename; // no choice, no pause
    }

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
        if (c == 13)
            c = '1'; // default to first
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
    snprintf(chosenFilename, sizeof(chosenFilename), "%s", dirFilenames[n]);

    for (i = 0; i < 10; ++i)
        free(dirFilenames[i]);

    terminal->write('\r');
    terminal->write(chosenFilename);
    terminal->write('\r');

    unsigned long start = micros();
    while (true) {
#ifdef _WINDOWS
        terminal->CheckPaintFrame(micros());
#endif
        delay(20);
        if ((micros() - start) >= 750000)
            break;
    }
    terminal->clearScreen();

    return chosenFilename;
}

bool MinimumMemory::SaveState(byte*& state, size_t& size) const
{
    size = (size_t)64 * 1024 + 8;
    state = new byte[size];
    if (state == 0) {
        size = 0;
        return false;
    }
    memcpy(state, ram, (size_t)64 * 1024);
    state[size - 8] = (byte)ramsize;
    state[size - 7] = ramsize >> 8;
    state[size - 6] = (byte)romsize;
    state[size - 5] = romsize >> 8;
    state[size - 4] = (byte)romaddr;
    state[size - 3] = romaddr >> 8;
    state[size - 2] = (byte)serialaddr;
    state[size - 1] = serialaddr >> 8;
    return true;
}

bool MinimumMemory::RestoreState(byte* state, size_t size)
{
    if (size != (size_t)64*1024 + 8)
        return false;
    if (state == 0)
        return false;
    memcpy(ram, state, (size_t)64 * 1024);
    ramsize = state[size - 8] | (state[size - 7] << 8);
    romsize = state[size - 6] | (state[size - 5] << 8);
    romaddr = state[size - 4] | (state[size - 3] << 8);
    serialaddr = state[size - 2] | (state[size - 1] << 8);
    return true;
}

bool EmuMinimum::SaveState(byte*& state, size_t& size)
{
    byte* cpu_state;
    size_t cpu_size;
    if (!Emu6502::SaveState(cpu_state, cpu_size))
        return false;

    byte* mem_state;
    size_t mem_size;
    if (!((MinimumMemory*)memory)->SaveState(mem_state, mem_size)) {
        delete[] cpu_state;
        size = 0;
        return false;
    }

    byte* uart_state;
    size_t uart_size;
    if (!((MinimumMemory*)memory)->uart->SaveState(uart_state, uart_size)) {
        delete[] cpu_state;
        delete[] mem_state;
        size = 0;
        return false;
    }

    byte* term_state;
    size_t term_size;
    if (!((MinimumMemory*)memory)->terminal->SaveState(term_state, term_size)) {
        delete[] cpu_state;
        delete[] mem_state;
        delete[] uart_state;
        size = 0;
        return false;
    }

    size_t base_size = 3;
    size = base_size + cpu_size + mem_size + uart_size + term_size;
    state = new byte[size];
    if (state == 0 || size < 2) {
        size = 0;
        delete[] cpu_state;
        delete[] mem_state;
        delete[] uart_state;
        delete[] term_state;
        return false;
    }

    state[0] = 0;
    state[1] = 0;
    state[2] = 1; // minimum emulation
    memcpy(state + base_size, cpu_state, cpu_size);
    memcpy(state + base_size + cpu_size, mem_state, mem_size);
    memcpy(state + base_size + cpu_size + mem_size, uart_state, uart_size);
    memcpy(state + base_size + cpu_size + mem_size + uart_size, term_state, term_size);

    delete[] cpu_state;
    delete[] mem_state;
    delete[] uart_state;
    delete[] term_state;

    return true;
}

bool EmuMinimum::RestoreState(byte* state, size_t size)
{
    size_t cpu_size = 17;
    size_t mem_size = (size_t)64 * 1024 + 8;
    size_t uart_size = 2;
    size_t term_size = (size_t)100 * 30 + 3;
    size_t base_size = 3;
    if (size != base_size + cpu_size + mem_size + uart_size + term_size)
        return false;
    if (state == 0)
        return false;
    if (state[0] != 0 || state[1] != 0) // filler (was going to be placeholder address for D64)
        return false;
    if (state[2] != 1) // minimum emulation
        return false;
    if (!Emu6502::RestoreState(state + base_size, cpu_size))
        return false;
    if (!((MinimumMemory*)memory)->RestoreState(state + base_size + cpu_size, mem_size))
        return false;
    if (!((MinimumMemory*)memory)->uart->RestoreState(state + base_size + cpu_size + mem_size, uart_size))
        return false;
    if (!((MinimumMemory*)memory)->terminal->RestoreState(state + base_size + cpu_size + mem_size + uart_size, term_size))
        return false;
    return true;
}

void EmuMinimum::ProcessSpecialKey(byte key)
{
    if (key == 0xF2 || key == 0xF3)
        LoadOrSaveState(key);
}

void EmuMinimum::LoadOrSaveState(byte key)
{
	if (key != 0xF2 && key != 0xF3)
		return;

	byte* state;
	size_t state_size;
	if (!SaveState(state, state_size))
		return;
	int choice = (key == 0xF2) ? 1 : 0; // LOAD, SAVE, N
	int n = 1;
	bool counting = false;
	Terminal* terminal = ((MinimumMemory*)memory)->terminal;
	while (true) {
		terminal->clearScreen();
		terminal->write("MINIMUM 6502 STATE\n\r");
		terminal->write("\n\r");
		terminal->write((choice == 0) ? '[' : ' ');
		terminal->write("load");
		terminal->write((choice == 0) ? ']' : ' ');
		terminal->write("\n\r");
		terminal->write((choice == 1) ? '[' : ' ');
		terminal->write("save");
		terminal->write((choice == 1) ? ']' : ' ');
		terminal->write("\n\r");
		char buffer[3];
		snprintf(buffer, sizeof(buffer), "%02d", n);
		terminal->write((choice == 2) ? '[' : ' ');
        terminal->write(counting ? '_' : ' ');
		terminal->write(buffer);
        terminal->write(counting ? '_' : ' ');
        terminal->write((choice == 2) ? ']' : ' ');
		char c;
		while (!terminal->read(c)) {
#ifdef _WINDOWS      
			terminal->CheckPaintFrame(micros());
#endif    
		}
		c = c & 0x7F;
		if (c == 13) {
			if (choice == 2)
				counting = !counting;
			else
				break;
		}
		if (c == 10) {
			if (counting) {
				if (n > 0)
					--n;
			}
			else if (choice < 2)
				++choice;
		}
		if (c == 11) {
			if (counting) {
				if (n < 99)
					++n;
			}
			else if (choice > 0)
				--choice;
		}
		if (c == 3) {
			choice = -1;
			break;
		}
	}

	if (choice == 0)
		ResetStateFromDisk(state, state_size, n);
	else if (choice == 1)
		SaveStateToDisk(state, state_size, n);

	if (state == 0)
		return;

	RestoreState(state, state_size);
	delete[] state;
}

void EmuMinimum::ResetStateFromDisk(byte*& state, size_t& state_size, int state_num)
{
    if (state_num < 0 || state_num > 99)
        return;

    char filename[20];
    snprintf(filename, sizeof(filename), "/snapshots/state%03d", state_num);
 
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    fs::File file = FFat.open(filename);
#else
    File file = SD.open(filename);
#endif
    file.read(state, (int)state_size);
    file.close();
}

void EmuMinimum::SaveStateToDisk(byte* state, size_t state_size, int state_num)
{
    if (state_num < 0 || state_num > 99)
        return;

    char filename[20];
    snprintf(filename, sizeof(filename), "/snapshots/state%03d", state_num);

#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    fs::File file = FFat.open(filename);
#else
    File file = SD.open(filename, FILE_WRITE);
#endif
    file.write(state, (int)state_size);
    file.close();
}
