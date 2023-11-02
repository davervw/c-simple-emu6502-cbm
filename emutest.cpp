// emutest.cpp - Emulator for 6502 Tests
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C Portable Version)
// C64/6502 Emulator for Teensy
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

#include "emu6502.h"

#ifdef TEST6502 // see emu6502.h

// plain 6502, no I/O, just memory to run test code
// expecting rom testfile similar to 6502_functional_test.bin from https://github.com/Klaus2m5/6502_65C02_functional_tests
// expected behavior is
// 0) loads all memory starting at address 0x0000 through address 0xFFFF including NMI, RESET, IRQ vectors at end of memory
// 1) start address of tests is at 0x400 manually patched by ExecutePatch(), not RESET vector
// 2) active test number stored at 0x200
// 3) failed test branches with BNE to same instruction to indicate cannot continue
// 4) IRQs must not be active or IRQ vector catch will fail tests
// 5) successful completion jumps to same instruction to indicate completion

#include "emuc64.h"
#include <arduino.h>

// locals
static int start = 1;
static int last_test = -1;
static byte ram[64 * 1024];
static const byte rom[64 * 1024] = {
// for 6502 tests MUST insert hex characters (bytes) from
// https://github.com/Klaus2m5/6502_65C02_functional_tests/blob/master/bin_files/6502_functional_test.bin
// otherwise will probably BRK from zeroed memory
// rom not included here due to licensing differences
};

void C64_Init(void)
{
	for (int i = 0; i < 65536; ++i)
		ram[i] = rom[i];
}

byte GetMemory(ushort addr)
{
  if (addr < 0x8000)
    return ram[addr];
  else
    return rom[addr];
}

void SetMemory(ushort addr, byte value)
{
  if (addr < 0x8000)
    ram[addr] = value;
}

bool ExecutePatch(void)
{
    if (start)
    {
        PC = 0x0400; // start address of tests are 0400
        Serial.print("Start\n");
        start = false;
    }
    if (GetMemory(PC) == 0xD0/*BNE*/ && !Z && GetMemory((ushort)(PC + 1)) == 0xFE/*-2*/)
    {
        Serial.printf("%04X Test %02X FAIL\n", PC, GetMemory(0x200));
        while(1) {}
    }
    if (GetMemory(PC) == 0x4C/*JMP*/
        && ((GetMemory((ushort)(PC + 1)) == (PC & 0xFF) && GetMemory((ushort)(PC + 2)) == (PC >> 8))
        || (GetMemory((ushort)(PC + 1)) == 0x00 && GetMemory((ushort)(PC + 2)) == 0x04))
        )
    {
        Serial.printf("%04X COMPLETED SUCCESS\n", PC);
        while(1) {}
    }
    if (GetMemory(0x200) != last_test)
    {
        last_test = GetMemory(0x200);
        Serial.printf("%04X Starting test %02X\n", PC, last_test);
    }  
    return false; // execute normally
}

#endif
