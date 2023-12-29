// emutest.cpp - Test Suite 6502 Emulator
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C++ Portable Version)
// C64/6502 Emulator for Microsoft Windows Console
//
// MIT License
//
// Copyright (c) 2023 by David R. Van Wagner
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

// plain 6502, no I/O, just memory to run test code

// expecting input testfile similar to 6502_functional_test.bin from https://github.com/Klaus2m5/6502_65C02_functional_tests
// expected behavior is
// 0) loads all memory starting at address 0x0000 through address 0xFFFF including NMI, RESET, IRQ vectors at end of memory
// 1) start address of tests is at 0x400 manually patched by ExecutePatch(), not RESET vector
// 2) active test number stored at 0x200
// 3) failed test branches with BNE to same instruction to indicate cannot continue
// 4) IRQs must not be active or IRQ vector catch will fail tests
// 5) successful completion jumps to same instruction to indicate completion

#include "emu6502.h"
#include "emucbm.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern const char* StartupPRG;

#include "emutest.h"

static bool start = true;
static int last_test = -1;

EmuTest::EmuTest()
	: Emu6502(new TestMemory("roms/test/6502test.bin"))
{
    //trace = true;
}

EmuTest::~EmuTest()
{
}

byte EmuTest::GetMemory(ushort addr)
{
	return memory->read(addr);
}

void EmuTest::SetMemory(ushort addr, byte value)
{
	memory->write(addr, value);
}

bool EmuTest::ExecutePatch()
{
    if (start)
    {
        PC = 0x400;
        start = false;
        return true;
    }
    if (GetMemory(PC) == 0xD0/*BNE*/ && !Z && GetMemory((ushort)(PC + 1)) == 0xFE)
    {
        printf("%04X Test FAIL\n", PC);
        while(1) {} // loop forever
    }
    if ( GetMemory(PC) == 0x4C/*JMP*/
        && ((GetMemory((ushort)(PC + 1)) == (PC & 0xFF) && GetMemory((ushort)(PC + 2)) == (PC >> 8))
            || (GetMemory((ushort)(PC + 1)) == 0x00 && GetMemory((ushort)(PC + 2)) == 0x04) )
        )
    {
        printf("%04X COMPLETED SUCCESS\n", PC);
        quit = true;
        while(1) {} // loop forever
    }
    if (GetMemory(0x200) != last_test)
    {
        last_test = GetMemory(0x200);
        printf("%04X Starting test %02X\n", PC, last_test);
        return false;
    }
	return false;
}

static const byte rom[64 * 1024] = {
// for 6502 tests MUST insert hex characters (bytes) from
// https://github.com/Klaus2m5/6502_65C02_functional_tests/blob/master/bin_files/6502_functional_test.bin
// otherwise will probably BRK from zeroed memory
// rom not included here due to licensing differences
};

EmuTest::TestMemory::TestMemory(const char* filename)
{
	const int ram_size = 65536;
	ram = new unsigned char[ram_size];
	memset(ram, 0, ram_size);
  memcpy(ram, rom, ram_size);
	//EmuCBM::File_ReadAllBytes(ram, ram_size, filename);
}

EmuTest::TestMemory::~TestMemory()
{
	delete[] ram;
}

byte EmuTest::TestMemory::read(ushort addr)
{
	return ram[addr];
}

void EmuTest::TestMemory::write(ushort addr, byte value)
{
	if (addr < 0x8000)
		ram[addr] = value;
}
