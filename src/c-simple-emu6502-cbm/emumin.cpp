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
            snprintf(buffer, sizeof(buffer), "%d. %s %d(%X) bytes\r", ++i, entry.name(), entry.size(), entry.size());
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
        if ((micros() - start) >= 2000000)
            break;
    }
    terminal->clearScreen();

    return chosenFilename;
}
