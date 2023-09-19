// emuted.cpp - Class EmuTed - Commodore TED Emulator (C16, Plus/4, etc.)
//
////////////////////////////////////////////////////////////////////////////////
//
// simple-emu-c64
// C64/6502 Emulator for Microsoft Windows Console
//
// MIT License
//
// Copyright (c) 2022 by David R. Van Wagner
// davevw.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////
//
// This is a 6502 Emulator, designed for running Commodore computers in text mode, 
//   with only a few hooks: CHRIN-$FFCF/CHROUT-$FFD2, and minimal I/O to function
// Useful as is in current state as a simple 6502 emulator and BASIC console
//
// LIMITATIONS: See EmuC64
//
// MEMORY MAP:
//   $0000-$3FFF 16K RAM, note repeats $4000-$7FFF, $8000-$BFFF, $C000-$FFFF
//   $0800-$0BE7 Color RAM bit 7 blinking, bits 6..4 luminance, bits 3..0 color
//   $0C00-$0FE7 Video RAM (characters)
//   ($0000-$7FFF 32K RAM, note repeats $8000-$FFFF)
//   ($0000-$FFFF 64K RAM minus non-banked portions, upper RAM can be banked to ROM)
//   $8000-$BFFF BASIC ROM
//   $FC00-$FCFF NON-BANKED KERNAL ROM (but can be banked to RAM)
//   $FD00-$FF3F I/O Registers (never banked, not even to RAM)
//   $FDD0-$FDDF ROM config addresses (A1/A0: BASIC/FUNCT/CART/RESV, A3/A2: KERNAL/FUNCT/CART/RESV)
//   $FF3E write banks to ROM
//   $FF3F write banks to RAM
//   $C000-$FFFF KERNAL ROM (minus I/O, can be banked to RAM)
//   $D000-$DFFF area of KERNAL ROM containing character ROM images
//
// Requires user provided Commodore 16 BASIC/KERNAL ROMs (e.g. from VICE)
//   as they are not provided, others copyrights may still be in effect.
//
// ROM has easter egg for credits
//   SYS 52650
//
// Function ROMs not implemented 
// (because not supporting screen editing and key scan codes yet)
// but this looks interesting for future reference:
//   https://www.rift.dk/commodore-16-internal-function-rom/
//
// Credits to
//   Commodore Source https://github.com/mist64/cbmsrc
//   C16 and Plus/4 Memory Map https://www.floodgap.com/retrobits/ckb/secret/264memory.txt
////////////////////////////////////////////////////////////////////////////////

#include "emuted.h"

// externals (globals)
extern const char* StartupPRG;
extern int main_go_num;
    
EmuTed::EmuTed(int ram_size) : EmuCBM(new TedMemory(ram_size*1024))
{
  startup_state = 0;
  go_state = 0;
}

EmuTed::~EmuTed()
{
}

bool EmuTed::ExecutePatch()
{
    if (PC == 0x8703 || PC == LOAD_TRAP) // READY
    {
        go_state = 0;

        if (startup_state == 0 && (StartupPRG != 0 || PC == LOAD_TRAP))
        {
            bool is_basic;
            if (PC == LOAD_TRAP)
            {
                is_basic = (
                    FileVerify == false
                    && FileSec == 0 // relative load, not absolute
                    && LO(FileAddr) == GetMemory(43) // requested load address matches BASIC start
                    && HI(FileAddr) == GetMemory(44));
                SetMemory(0xFF3F, 0); // switch to RAM
                byte err;
                bool success = FileLoad(&err);
                SetMemory(0xFF3E, 0); // switch to ROM
                if (!success)
                {
                    //System.Diagnostics.Debug.WriteLine(string.Format("FileLoad() failed: err={0}, file {1}", err, StartupPRG));
                    C = true; // signal error
                    SetA(err); // FILE NOT FOUND or VERIFY

                    // so doesn't repeat
                    StartupPRG = 0;
                    LOAD_TRAP = -1;

                    return true; // overriden, and PC changed, so caller should reloop before execution to allow breakpoint/trace/ExecutePatch/etc.
                }
            }
            else
            {
                FileName = StartupPRG;
                FileAddr = (ushort)(GetMemory(43) | (GetMemory(44) << 8));
                SetMemory(0xFF3F, 0); // switch to RAM
                is_basic = LoadStartupPrg();
                SetMemory(0xFF3E, 0); // switch to ROM
            }

            StartupPRG = 0;

            if (is_basic)
            {
                // initialize first couple bytes (may only be necessary for UNNEW?)
                ushort addr = (ushort)(GetMemory(43) | (GetMemory(44) << 8));
                SetMemory(addr, 1);
                SetMemory((ushort)(addr + 1), 1);

                startup_state = 1; // should be able to regain control when returns...

                return ExecuteJSR(0x8818); // LINKPRG
            }
            else
            {
                LOAD_TRAP = -1;
                X = LO(FileAddr);
                Y = HI(FileAddr);
                C = false;
            }
        }
        else if (startup_state == 1)
        {
            ushort addr = (ushort)(GetMemory(0x22) | (GetMemory(0x23) << 8) + 2);
            SetMemory(45, (byte)addr);
            SetMemory(46, (byte)(addr >> 8));

            SetA(0);

            startup_state = 2; // should be able to regain control when returns...

            return ExecuteJSR(0x8A98); // CLEAR/CLR
        }
        else if (startup_state == 2)
        {
            if (PC == LOAD_TRAP)
            {
                X = LO(FileAddr);
                Y = HI(FileAddr);
            }
            else
            {
                //CBM_Console.Push("RUN\r");
                PC = 0x8706; // skip READY message, but still set direct mode, and continue to MAIN
            }
            C = false; // signal success
            startup_state = 0;
            LOAD_TRAP = -1;
            return true; // overriden, and PC changed, so caller should reloop before execution to allow breakpoint/trace/ExecutePatch/etc.
        }
    }
    else if (PC == 0x8C77) // Execute after GO
    {
        if (go_state == 0 && A >= (byte)'0' && A <= (byte)'9')
        {
            go_state = 1;
            return ExecuteJSR(0x8E3E); // Get integer into $14/$15
        }
        else if (go_state == 1)
        {
            main_go_num = (ushort)(GetMemory(0x14) + (GetMemory(0x15) << 8));
            quit = true;
            return true;
        }
        else
            go_state = 0;
    }
    else if (PC == 0xFFBD) // SETNAM
    {
        SetMemory(0xFF3F, 0); // switch to RAM
        bool retvalue = EmuCBM::ExecutePatch();
        SetMemory(0xFF3E, 0); // switch to ROM
        return retvalue;
    }
    return EmuCBM::ExecutePatch();
}

// note ram starts at 0x0000
const int basic_addr = 0x8000;
const int basic_rom_length = 0x4000;
const int kernal_addr = 0xC000;
const int kernal_rom_length = 0x4000;
const int io_addr = 0xFD00; // IO cannot be banked ever
const int io_length = 0x0240; // note hole in IO: FF20-FF3D, RAM or ROM?
const int nonbank_kernal = 0xFC00; // can be banked to RAM, but not to other ROMs
const int nonbank_len = 0x0100;
const int config_addr = 0xFDD0;
const int config_len = 0x0010;

// ROM configuration in I/O
// A1/A0: 00=BASIC LO, 01=FUNCTION LO, 10=CARTRIDGE LO, 11=RESERVED LO
// A3/A2: 00=KERNAL HI, 01=FUNCTION HI, 10=CARTRIDGE HI, 11=RESERVED HI
// FDD0 BASIC LO/KERNAL HI
// FDD1 FUNCTION LO/KERNAL HI
// FDD2 CARTRIDGE LO/KERNAL HI
// FDD3 RESERVED LO/KERNAL HI
// FDD4 BASIC LO/FUNCTION HI
// FDD5 FUNCTION LO/FUNCTION HI
// FDD6 CARTRIDGE LO/FUNCTION HI
// FDD7 RESERVED LO/FUNCTION HI
// FDD8 BASIC LO/CARTRIDGE HI
// FDD9 FUNCTION LO/CARTRIDGE HI
// FDDA CARTRIDGE LO/CARTRIDGE HI
// FDDB RESERVED/CARTRIDGE HI
// FDDC BASIC LO/RESERVED HI
// FDDD FUNCTION LO/RESERVED HI
// FDDE CARTRIDGE LO/RESERVED HI
// FDDF RESERVED LO/RESERVED HI

EmuTed::TedMemory::TedMemory(int size)
{
    rom_enabled = true; // FF3E=rom & FF3F=ram
    rom_config = 0;

    ram_size = size;
    if (ram_size < 32 * 1024)
        ram_size = 16 * 1024;
    else if (ram_size < 64 * 1024)
        ram_size = 32 * 1024;
    else
        ram_size = 64 * 1024;

    ram = new byte[ram_size];

    basic_rom = new byte[basic_rom_length];
    EmuCBM::File_ReadAllBytes(basic_rom, basic_rom_length, "roms/ted/basic");
    kernal_rom = new byte[kernal_rom_length];
    EmuCBM::File_ReadAllBytes(kernal_rom, kernal_rom_length, "roms/ted/kernal");

    for (int i = 0; i < ram_size; ++i)
        ram[i] = 0;

    io = new byte[io_length];
    for (int i = 0; i < io_length; ++i)
        io[i] = 0;
}

EmuTed::TedMemory::~TedMemory()
{
    delete [] ram;
    delete [] basic_rom;
    delete [] kernal_rom;
    delete [] io;
}

byte EmuTed::TedMemory::read(ushort addr)
{
    if (addr == 0xFF08) // key matrix
        return 0xFF; // FF = no key, 7F = stop (open in monitor)
    if (addr >= io_addr && addr < io_addr + io_length)
        return io[addr - io_addr];
    else if (!rom_enabled || addr < basic_addr)
        return ram[addr & (ram_size - 1)]; // note RAM wraps around when less than 64K
    else if (((rom_config & 0x03) == 0) && rom_enabled && addr >= basic_addr && addr < basic_addr + basic_rom_length)
        return basic_rom[addr - basic_addr];
    else if (((rom_config & 0x0C) == 0) && rom_enabled && (addr >= kernal_addr && addr < kernal_addr + kernal_rom_length) || (addr >= nonbank_kernal && addr < nonbank_kernal + nonbank_len))
        return kernal_rom[addr - kernal_addr];
    else
        return 0xFF;
}

void EmuTed::TedMemory::write(ushort addr, byte value)
{
    if (addr == 0xFF3E)
        rom_enabled = true;
    else if (addr == 0xFF3F)
        rom_enabled = false;
    else if (addr >= config_addr && addr < config_addr + config_len)
        rom_config = addr & 0xF;
    else if (addr >= io_addr && addr < io_addr + io_length)
    {
        io[addr - io_addr] = value;
        // if (addr == 65301)
        //     ApplyColor();
        // else if (addr == 0xFF13)
        //     CBM_Console.Lowercase = (value & 4) != 0;
    }
    else
    {
        ram[addr & (ram_size - 1)] = value; // includes writing under rom, note RAM wraps around when less than 64K
        // if (addr == 194 || addr == 1339)
        //     ApplyColor();
        // else if (addr == 207)
        //     CBM_Console.InsertMode = (value != 0);
        // else if (addr == 203)
        //     CBM_Console.QuoteMode = (value != 0);
    }
}

    // private void ApplyColor()
    // {
    //     CBM_Console.Reverse = (this[194] != 0);
    //     if (CBM_Console.Color)
    //     {
    //         var bg_register = io[65301-io_addr];
    //         var bg_color = (TedColor)(bg_register & 0xF);
    //         var bg_luminance = (bg_register >> 4) & 7;
    //         var fg_color = (TedColor)(ram[1339] & 0xF);
    //         var fg_luminance = (ram[1339] >> 4) & 7;

    //         if (CBM_Console.Reverse && CBM_Console.Encoding != CBM_Console.CBMEncoding.petscii)
    //         {
    //             Console.BackgroundColor = ConvertToConsoleColor(fg_color, fg_luminance);
    //             Console.ForegroundColor = ConvertToConsoleColor(bg_color, bg_luminance);
    //         }
    //         else
    //         {
    //             Console.BackgroundColor = ConvertToConsoleColor(bg_color, bg_luminance);
    //             Console.ForegroundColor = ConvertToConsoleColor(fg_color, fg_luminance);
    //         }
    //     }
    //     else
    //     {
    //         if (CBM_Console.Reverse && CBM_Console.Encoding != CBM_Console.CBMEncoding.petscii)
    //         {
    //             Console.BackgroundColor = startup_fg;
    //             Console.ForegroundColor = startup_bg;
    //         }
    //         else
    //         {
    //             Console.ForegroundColor = startup_fg;
    //             Console.BackgroundColor = startup_bg;
    //         }
    //     }
    // }

enum TedColor
{
    Black = 0,
    Gray = 1,
    White = 1,
    Red = 2,
    Cyan = 3,
    Magenta = 4,
    Green = 5,
    Blue = 6,
    Yellow = 7,
    Orange = 8,
    Brown = 9,
    YellowGreen = 10,
    Pink = 11,
    BlueGreen = 12,
    LightBlue = 13,
    Purple = 14,
    LightGreen = 15
};

// ConsoleColor ConvertToConsoleColor(TedColor color, int luminance)
// {
//     switch (color)
//     {
//         case TedColor.Black:
//             return ConsoleColor.Black;
//         case TedColor.Gray:
//             if (luminance < 2)
//                 return ConsoleColor.DarkCyan;
//             else if (luminance < 4)
//                 return ConsoleColor.DarkGray;
//             else if (luminance < 7)
//                 return ConsoleColor.Gray;
//             else
//                 return ConsoleColor.White;
//         case TedColor.Red:
//             return ConsoleColor.Red;
//         case TedColor.Cyan:
//             return ConsoleColor.Cyan;
//         case TedColor.Magenta:
//             return ConsoleColor.DarkMagenta;
//         case TedColor.Green:
//             return ConsoleColor.DarkGreen;
//         case TedColor.Blue:
//             return ConsoleColor.DarkBlue;
//         case TedColor.Yellow:
//             return ConsoleColor.Yellow;
//         case TedColor.Orange:
//             return ConsoleColor.DarkYellow;
//         case TedColor.Brown:
//             return ConsoleColor.DarkRed;
//         case TedColor.YellowGreen:
//             return ConsoleColor.Green;
//         case TedColor.Pink:
//             return ConsoleColor.Magenta;
//         case TedColor.BlueGreen:
//             return ConsoleColor.DarkGreen;
//         case TedColor.LightBlue:
//             return ConsoleColor.Blue;
//         case TedColor.Purple:
//             return ConsoleColor.DarkMagenta;
//         case TedColor.LightGreen:
//             return ConsoleColor.Green;
//         default:
//             return ConsoleColor.Black;
//     }
// }
