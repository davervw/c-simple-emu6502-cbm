// emuc128.cs - Class EmuC128 - Commodore 128 Emulator
//
////////////////////////////////////////////////////////////////////////////////
//
// simple-emu-c64
// C64/6502 Emulator for Microsoft Windows Console
//
// MIT License
//
// Copyright (c) 2020-2022 by David R. Van Wagner
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////
//
// This is a 6502 Emulator, designed for running Commodore 128 text mode, 
//   with only a few hooks: CHRIN/CHROUT/COLOR-$D021/241/243/READY/GETIN/STOP
//   and RAM/ROM/IO banking from 6510, and C128 MMU
//   READY hook is used to load program specified on command line
//
// LIMITATIONS:
// Only keyboard/console I/O.  No text pokes, no graphics.  Just stdio.  
//   No key scan codes (197), or keyboard buffer (198, 631-640), but INPUT S$ works
// No keyboard color switching.  No border displayed.  No border color.
// No screen editing (gasp!) Just short and sweet for running C128 BASIC in 
//   terminal/console window via 8502 (6502) chip emulation in software
// No PETSCII graphic characters, only supports printables CHR$(32) to CHR$(126), 
//   and CHR$(147) clear screen, home/up/down/left/right, reverse on/off
// No timers.  No interrupts except BRK.  No NMI/RESTORE key.  ESC is STOP key.
//   but TI$/TI are simulated.
//
//   $00         On chip (8502) data direction register missing in this emulation
//   $01         On chip (8502) I/O register minimally implemented
//
//   $0002-$3FFF RAM (BANK 0 or BANK 1)
//
//   $4000-$7FFF BASIC ROM LO
//   $4000-$7FFF Banked RAM (BANK 0 or BANK 1)
//
//   $8000-$BFFF BASIC ROM HI
//   $8000-$BFFF Banked RAM (BANK 0 or BANK 1)
//
//   $C000-$FFFF Banked KERNAL/CHAR(DXXX) ROM
//   $C000-$FFFF Banked RAM (BANK 0 or BANK 1)
//
//   $D000-$D7FF I/O minimally implemented, reads as zeros
//   $D800-$DFFF VIC-II color RAM nybbles in I/O space (1K x 4bits)
//   $D000-$DFFF Banked Character ROM
//   $D000-$DFFF Banked RAM (BANK 0 or BANK 1)
//
// Requires user provided Commodore 128 BASIC/KERNAL ROMs (e.g. from VICE)
//   as they are not provided, others copyrights may still be in effect.
//
////////////////////////////////////////////////////////////////////////////////

#include "emuc128.h"

#include <string.h>
#ifdef WINDOWS
#else
#include <unistd.h>
#endif
#include <stdlib.h>

extern int main_go_num;

EmuC128::EmuC128()
    : EmuCBM(new C128Memory())
{
    c128memory = (C128Memory*)memory;
    File_ReadAllBytes(c128memory->basic_lo_rom, C128Memory::basic_lo_size, "roms/c128/basiclo");
    File_ReadAllBytes(c128memory->basic_hi_rom, C128Memory::basic_hi_size, "roms/c128/basichi");
    File_ReadAllBytes(c128memory->char_rom, C128Memory::chargen_size, "roms/c128/chargen");
    File_ReadAllBytes(c128memory->kernal_rom, C128Memory::kernal_size, "roms/c128/kernal");
}

EmuC128::~EmuC128()
{
}

static int startup_state = 0;
static bool esc_mode = false;

bool EmuC128::ExecutePatch()
{
    if (PC == 0xFFD2)
    {
        if (A == 27)
            esc_mode = !esc_mode;
        else if (esc_mode)
        {
            esc_mode = false;
            return false; // suppress output to Console
        }
        return EmuCBM::ExecutePatch();
    }
    if (GetMemory(PC) == 0x6C && GetMemory((ushort)(PC + 1)) == 0x30 && GetMemory((ushort)(PC + 2)) == 0x03) // catch JMP(LOAD_VECTOR), redirect to jump table
    {
        int addr128k = 0x330;
        if (c128memory->IsRam(addr128k, false) && addr128k == 0x330)
        {
            CheckBypassSETLFS();
            CheckBypassSETNAM();
            // note: A register has same purpose LOAD/VERIFY
            X = GetMemory(0xC3);
            Y = GetMemory(0xC4);
            PC = 0xFFD5; // use KERNAL JUMP TABLE instead, so LOAD is hooked by base
            return true; // re-execute
        }
    }
    if (GetMemory(PC) == 0x6C && GetMemory((ushort)(PC + 1)) == 0x32 && GetMemory((ushort)(PC + 2)) == 0x03) // catch JMP(SAVE_VECTOR), redirect to jump table
    {
        int addr128k = 0x332;
        if (c128memory->IsRam(addr128k, false) && addr128k == 0x332)
        {
            CheckBypassSETLFS();
            CheckBypassSETNAM();
            X = GetMemory(0xAE);
            Y = GetMemory(0xAF);
            A = 0xC1;
            PC = 0xFFD8; // use KERNAL JUMP TABLE instead, so SAVE is hooked by base
            return true; // re-execute
        }
    }

    // Note: BANK # (0-15) for file i/o is in $C6
    // Note: BANK # (0-15) for filename is in $C7
    // Note: BANK # to MMU CR translation table in Kernal at $F7F0
    if (PC == 0xFFBD && c128memory->IsKernal(PC)) // SETNAM
    {
        // set to name BANK (reference $FF68 JSETBNK)
        byte save_mcr = GetMemory(0xFF00);
        SetMemory(0xFF00, 0); // switch in KERNAL where mcr table is located
        SetMemory(0xFF00, GetMemory((ushort)(0xF7F0 + GetMemory(0xC7)))); // switch to name bank

        bool result = EmuCBM::ExecutePatch(); // DELEGATE TO emucbm

        SetMemory(0xFF00, save_mcr); // restore MCR
        return result;
    }
    else if ((PC == 0xFFD5 || PC == 0xFFD8) && c128memory->IsKernal(PC)) // LOAD OR SAVE
    {
        // set to data i/o BANK
        byte save_mcr = GetMemory(0xFF00);
        SetMemory(0xFF00, 0); // switch in KERNAL where mcr table is located
        SetMemory(0xFF00, GetMemory((ushort)(0xF7F0 + GetMemory(0xC6)))); // switch to i/o bank

        bool result = EmuCBM::ExecutePatch(); // DELEGATE TO emucbm

        SetMemory(0xFF00, save_mcr); // restore MCR
        return result;
    }
    else if ((PC == 0x4D37 || PC == LOAD_TRAP) && (c128memory->IsBasicLow(PC) || c128memory->IsBasicHigh(PC))) // READY
    {
        if (startup_state == 0 && (StartupPRG != 0 || PC == LOAD_TRAP))
        {
            bool is_basic;
            if (PC == LOAD_TRAP)
            {
                is_basic = (
                    FileVerify == false
                    && FileSec == 0 // relative load, not absolute
                    && LO(FileAddr) == GetMemory(45) // requested load address matches BASIC start
                    && HI(FileAddr) == GetMemory(46));
                byte err;
                if (FileLoad(&err))
                {
                    SetMemory(0xAE, (byte)FileAddr);
                    SetMemory(0xAF, (byte)(FileAddr >> 8));
                }
                else
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
                FileAddr = (ushort)(GetMemory(45) | (GetMemory(46) << 8));
                is_basic = LoadStartupPrg();
                //GetMemory(0xAE] = (byte)FileAddr;
                //GetMemory(0xAF] = (byte)(FileAddr >> 8);
            }

            StartupPRG = 0;

            if (is_basic)
            {
                // initialize first couple bytes (may only be necessary for UNNEW?)
                ushort addr = (ushort)(GetMemory(45) | (GetMemory(46) << 8));
                SetMemory(addr, 1);
                SetMemory((ushort)(addr + 1), 1);

                startup_state = 1; // should be able to regain control when returns...

                return ExecuteJSR(0xAF87); // LINKPRG
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
            ushort addr = (ushort)(GetMemory(0x24) | (GetMemory(0x25) << 8) + 2);
            SetMemory(47, (byte)addr);
            SetMemory(48, (byte)(addr >> 8));

            SetA(0);

            startup_state = 2; // should be able to regain control when returns...

            return ExecuteJSR(0x51F8); // CLEAR/CLR
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
                //CBM_Console_Push("RUN\r");
                PC = 0xA47B; // skip READY message, but still set direct mode, and continue to MAIN
            }
            C = false; // signal success
            startup_state = 0;
            LOAD_TRAP = -1;
            return true; // overriden, and PC changed, so caller should reloop before execution to allow breakpoint/trace/ExecutePatch/etc.
        }
    }
    else if (PC == 0x05A4A) // GO next token is not TO, used to catch 2001 as ASCII
    {
        ushort addr = (ushort)(GetMemory(0x3D) | (GetMemory(0x3E) << 8)); // pointer to current token in buffer
        static char s[81];
        s[0] = 0;
        while (strlen(s) < 80) // some limit
        {
            char c = (char)GetMemory(addr++);
            if (c >= '0' && c <= '9')
                s[strlen(s)] = c;
            else if (c == 0 || strlen(s) > 0)
                break;
        }
        int go_num = atoi(s);
        if (go_num == 2001)
        {
            main_go_num = go_num;
            quit = true;
            return true;
        }
    }
    else if (PC == 0x05A4D) // GO value expression evaluated to byte stored in .X, catch other byte values that are not 64
    {
        if (X != 64)
        {
            main_go_num = X;
            quit = true;
            return true;
        }
    }

    if (main_go_num == 64)
    {
        quit = true;
        return true;
    }

    return EmuCBM::ExecutePatch();
}

void EmuC128::CheckBypassSETNAM()
{
    byte save_mcr = GetMemory(0xFF00);
    SetMemory(0xFF00, 0); // switch in KERNAL where mcr table is located

    // In case caller bypassed calling SETNAM, get from lower memory
    byte name_len = GetMemory(0xB7);
    ushort name_addr = (ushort)(GetMemory(0xBB) | (GetMemory(0xBC) << 8));
    static char name[256];

    SetMemory(0xFF00, GetMemory((ushort)(0xF7F0 + GetMemory(0xC7)))); // switch to name bank

    for (byte i = 0; i < name_len; ++i)
        name[i] = (char)GetMemory((ushort)(name_addr + i));
    name[name_len] = 0;

    SetMemory(0xFF00, save_mcr); // restore MCR

    if (strcmp(FileName, name) != 0)
    {
        //System.Diagnostics.Debug.WriteLine(string.Format("bypassed SETNAM {0}", name.ToString()));
        FileName = name;
    }
}

void EmuC128::CheckBypassSETLFS()
{
    // In case caller bypassed calling SETLFS, get from lower memory
    if (
        FileNum != GetMemory(0xB8)
        || FileDev != GetMemory(0xBA)
        || FileSec != GetMemory(0xB9)
        )
    {
        FileNum = GetMemory(0xB8);
        FileDev = GetMemory(0xBA);
        FileSec = GetMemory(0xB9);
        //System.Diagnostics.Debug.WriteLine(string.Format("bypassed SETLFS {0},{1},{2}", FileNum, FileDev, FileSec));
    }
}

///////////////////////////////////////////////////////////////////////

// note ram starts at 0x0000
static const int ram_size = 0x20000;
static const int basic_lo_addr = 0x4000;
static const int basic_hi_addr = 0x8000;
static const int kernal_addr = 0xC000;
static const int io_addr = 0xD000;
static const int io_size = 0x1000;
static const int color_addr = 0xD800;
static const int color_size = 0x0400;
static const int mmu_addr = 0xD500;
static const int mmu_size = 0xC;
static const int chargen_addr = io_addr;

C128Memory::C128Memory()
{
    ram = new byte[ram_size];

    for (int i = 0; i < ram_size; ++i)
        ram[i] = 0;

    basic_lo_rom = new byte[basic_lo_size];
    basic_hi_rom = new byte[basic_hi_size];
    char_rom = new byte[chargen_size];
    kernal_rom = new byte[kernal_size];

    io = new byte[io_size];
    for (int i = 0; i < io_size; ++i)
        io[i] = 0x0;

    io[mmu_addr - io_addr] = 0; // default MMU CR
    io[0xD505 - io_addr] = 0xB9; // 40/80 up, no /GAME, no /EXROM, C128 mode, Fast serial out, 8502 select
    io[0xD506 - io_addr] = 0; // no common RAM at startup
    io[0xD507 - io_addr] = 0; // zero page default
    io[0xD508 - io_addr] = 0; // zero page bank
    io[0xD509 - io_addr] = 1; // stack page default
    io[0xD50A - io_addr] = 0; // stack page bank
    io[0xD50B - io_addr] = 0x20; // MMU verison register value 128K, verison 0

    io[0xDC00 - io_addr] = 0xFF; // CIA #1 PORT A
    io[0xDC01 - io_addr] = 0xFF; // CIA #1 PORT B
    io[0xDD00 - io_addr] = 0xFF; // CIA #2 PORT A including SERIAL CLK/DATA INPUT pulled HIGH, no devices present
    io[0xDD01 - io_addr] = 0xFF; // CIA #2 PORT B

    vdc = new VDC8563();
}

C128Memory::~C128Memory()
{
    delete[] basic_lo_rom;
    delete[] basic_hi_rom;
    delete[] char_rom;
    delete[] kernal_rom;
    
    delete[] ram;
    delete[] io;
    delete[] color_nybles;

    delete vdc;
}

static void ApplyColor()
{
//    CBM_Console.Reverse = (this[243] != 0);
//
//    if (CBM_Console.Color)
//    {
//        if (CBM_Console.Reverse && CBM_Console.Encoding != CBM_Console.CBMEncoding.petscii)
//        {
//            Console.BackgroundColor = ToConsoleColor(this[241]);
//            Console.ForegroundColor = ToConsoleColor(this[0xD021]);
//        }
//        else
//        {
//            Console.ForegroundColor = ToConsoleColor(this[241]);
//            Console.BackgroundColor = ToConsoleColor(this[0xD021]);
//        }
//    }
//    else
//    {
//        if (CBM_Console.Reverse && CBM_Console.Encoding != CBM_Console.CBMEncoding.petscii)
//        {
//            Console.BackgroundColor = startup_fg;
//            Console.ForegroundColor = startup_bg;
//        }
//        else
//        {
//            Console.ForegroundColor = startup_fg;
//            Console.BackgroundColor = startup_bg;
//        }
//    }
}

//private ConsoleColor ToConsoleColor(byte CommodoreColor)
//{
//    switch (CommodoreColor & 0xF)
//    {
//    case 0: return ConsoleColor.Black;
//    case 1: return ConsoleColor.White;
//    case 2: return ConsoleColor.Red;
//    case 3: return ConsoleColor.Cyan;
//    case 4: return ConsoleColor.DarkMagenta;
//    case 5: return ConsoleColor.DarkGreen;
//    case 6: return ConsoleColor.DarkBlue;
//    case 7: return ConsoleColor.Yellow;
//    case 8: return ConsoleColor.DarkYellow;
//    case 9: return ConsoleColor.DarkRed;
//    case 10: return ConsoleColor.Magenta;
//    case 11: return ConsoleColor.DarkCyan;
//    case 12: return ConsoleColor.DarkGray;
//    case 13: return ConsoleColor.Green;
//    case 14: return ConsoleColor.Blue;
//    case 15: return ConsoleColor.Gray;
//    default: throw new InvalidOperationException("Missing case number in ToConsoleColor");
//    }
//}

static void CheckLowercase()
{
//    CBM_Console.Lowercase = ((ram[0xD7] & 0x80) == 0) && ((ram[0xA2C] & 2) != 0)
//        || ((ram[0xD7] & 0x80) != 0) && ((ram[0xF1] & 0x80) != 0);
}

byte C128Memory::read(ushort addr)
{
    int addr128k = addr;
    if (addr >= 0xFF00 && addr <= 0xFF04)
        return io[mmu_addr + (addr & 0xF) - io_addr];
    else if (IsRam(addr128k, false))
        return ram[addr128k];
    else if (IsIO(addr))
    {
        if (IsColor(addr))
            return (byte)((io[addr - io_addr] & 0xF) | 0xF0);
        else if (addr == 0xD011)
            io[addr - io_addr] ^= 0x80; // toggle 9th raster line bit, so seems like raster is moving
        else if (addr == 0xD600)
            return vdc->GetAddressRegister();
        else if (addr == 0xD601)
            return vdc->GetDataRegister();

        return io[addr - io_addr];
    }
    else if (IsBasicLow(addr))
        return basic_lo_rom[addr - basic_lo_addr];
    else if (IsBasicHigh(addr))
        return basic_hi_rom[addr - basic_hi_addr];
    else if (IsChargen(addr))
        return char_rom[addr - chargen_addr];
    else if (IsKernal(addr))
        return kernal_rom[addr - kernal_addr];
    else
        return 0xFF;
}

void C128Memory::write(ushort addr, byte value)
{
    if (addr == 0xFF00) // CR mirror
        io[mmu_addr - io_addr] = value; // CR
    else if (addr == 0xFF01) // LCRA
        io[mmu_addr - io_addr] = io[mmu_addr - io_addr + 1];
    else if (addr == 0xFF02) // LCRA
        io[mmu_addr - io_addr] = io[mmu_addr - io_addr + 2];
    else if (addr == 0xFF03) // LCRA
        io[mmu_addr - io_addr] = io[mmu_addr - io_addr + 3];
    else if (addr == 0xFF04) // LCRA
        io[mmu_addr - io_addr] = io[mmu_addr - io_addr + 4];
    else if (IsIO(addr))
    {
        if (addr == 0xD021) // background
        {
            io[addr - io_addr] = (byte)((value & 0xF) | 0xF0); // store value so can be retrieved
            ApplyColor();
        }
        else if (addr == 0xD505)
        {
            //System.Diagnostics.Debug.WriteLine($"Mode Configuration Register set 0x{value:X02}");
            if ((value & 0x40) != 0)
                main_go_num = 64;
        }
        else if (addr >= mmu_addr && addr < mmu_addr + mmu_size - 1) // MMU up to but not including version register
            io[addr - io_addr] = value;
        else if (addr == 0xD600)
            vdc->SetAddressRegister(value);
        else if (addr == 0xD601)
            vdc->SetDataRegister(value);
        // but do not set other I/O values
    }
    else
    {
        int addr128k = addr;
        if (IsRam(addr128k, true))
        {
            ram[addr128k] = value;
            if (addr128k == 241 || addr128k == 243)
                ApplyColor();
            else if (addr128k == 0xA2C || addr128k == 0xF1)
                CheckLowercase();
            //else if (addr128k == 244)
            //    CBM_Console_QuoteMode = (value != 0);
            //else if (addr128k == 245)
            //    CBM_Console_InsertMode = (value != 0);
        }
    }
}

bool C128Memory::IsChargen(ushort addr)
{
    byte mmu_cr = io[mmu_addr - io_addr];
    return (addr >= chargen_addr && addr < chargen_addr + chargen_size && (mmu_cr & 0x30) == 0);
}

bool C128Memory::IsKernal(ushort addr)
{
    byte mmu_cr = io[mmu_addr - io_addr];
    return (addr >= kernal_addr && !(addr >= chargen_addr && addr < chargen_addr + chargen_size) && (mmu_cr & 0x30) == 0);
}

bool C128Memory::IsBasicHigh(ushort addr)
{
    byte mmu_cr = io[mmu_addr - io_addr];
    return (addr >= basic_hi_addr && addr < basic_hi_addr + basic_hi_size && (mmu_cr & 0x0C) == 0);
}

bool C128Memory::IsBasicLow(ushort addr)
{
    byte mmu_cr = io[mmu_addr - io_addr];
    return (addr >= basic_lo_addr && addr < basic_lo_addr + basic_lo_size && (mmu_cr & 0x02) == 0);
}

bool C128Memory::IsColor(ushort addr)
{
    return IsIO(addr) && addr >= color_addr && addr < color_addr + color_size;
}

bool C128Memory::IsIO(ushort addr)
{
    byte mmu_cr = io[mmu_addr - io_addr];
    return (addr >= io_addr && addr < io_addr + io_size && (mmu_cr & 0x01) == 0);
}

bool C128Memory::IsRam(int& addr, bool isWrite = false)
{
    byte mmu_cr = io[mmu_addr - io_addr]; // MMU configuration register
    byte ram_cr = io[0xD506 - io_addr]; // RAM configuration register
    int page0_addr = (io[0xD507 - io_addr] | (io[0xD508 - io_addr] << 8)) << 8;
    int page1_addr = (io[0xD509 - io_addr] | (io[0xD50A - io_addr] << 8)) << 8;
    // note: ignore (mmu_cr & 0x80) != 0 because expansion RAM not implemented in hardware
    if (addr >= kernal_addr && (mmu_cr & 0x30) != 0x30 && !isWrite)
        return false;
    if (addr >= basic_hi_addr && addr < basic_hi_addr + basic_hi_size && (mmu_cr & 0x0C) != 0x0C && !isWrite)
        return false;
    if (addr >= basic_lo_addr && addr < basic_lo_addr + basic_lo_size && (mmu_cr & 0x02) != 0x02 && !isWrite)
        return false;
    if (addr >= io_addr && addr < io_addr + io_size && (mmu_cr & 0x01) != 0x01)
        return false;

    // bank 1
    if ((mmu_cr & 0x40) != 0)
        addr |= 0x10000;

    // remap/swap zero page and stack
    if (addr >= page0_addr && addr < page0_addr + 0x100)
    {
        addr = (addr & 0xFF) | (page0_addr & 0x10000);
    }
    else if (addr >= page1_addr && addr < page1_addr + 0x100)
    {
        addr = (addr & 0xFF) | 0x100 | (page1_addr & 0x10000);
    }
    else if ((ushort)addr < 0x100)
    {
        addr |= page0_addr;
    }
    else if ((ushort)addr >= 0x100 && (ushort)addr < 0x200)
    {
        addr = (addr & 0xFF) | page1_addr;
    }

    bool hasCommonRam = ((ram_cr & 0x0C) != 0);
    if (hasCommonRam && addr >= 0x10000)
    {
        int size;
        switch (ram_cr & 3)
        {
        case 0: size = 1024; break;
        case 1: size = 4096; break;
        case 2: size = 8192; break;
        case 3: size = 16384; break;
        default: throw "shouldn't happen";
        }

        bool isBottomShared = ((ram_cr & 4) != 0);
        bool isTopShared = ((ram_cr & 8) != 0);

        if (isBottomShared && (ushort)addr < size)
            addr = (ushort)addr; // common RAM is in BANK 0
        else if (isTopShared && (ushort)addr + size >= 0x10000)
            addr = (ushort)addr; // common RAM is in BANK 0
    }

    return true;
}

byte EmuC128::GetMemory(ushort addr)
{
    return memory->read(addr);
}

void EmuC128::SetMemory(ushort addr, byte value)
{
    memory->write(addr, value);
}

// VDC8563 ////////////////////////////////////////////////////////////

const int vdc_ram_size = 64 * 1024;
const int registers_size = 38;

VDC8563::VDC8563()
{
    registers = new byte[registers_size]
    {
        126, 80, 102, 73, 32, 224, 25, 29,
        252, 231, 160, 231, 0, 0, 0, 0,
        0, 0, 15, 228, 8, 0, 120, 232,
        32, 71, 240, 0, 63, 21, 79, 0,
        0, 0, 125, 100, 245, 63
    };

    vdc_ram = new byte[vdc_ram_size];
    memset(vdc_ram, 0, vdc_ram_size);
}

VDC8563::~VDC8563()
{
    delete[] registers;
    delete[] vdc_ram;
}

byte VDC8563::GetAddressRegister()
{
    if (ready)
    {
        return 128;
    }
    else
    {
        ready = true; // simulate delay in processing
        return 0x36;
    }
}

void VDC8563::SetAddressRegister(byte value)
{
    register_addr = value & 0x3F;
    if (register_addr < registers_size)
        data = registers[register_addr];
    else
        data = 0xFF;
    ready = false; // simulate delay in processing
}

byte VDC8563::GetDataRegister()
{
    if (ready)
    {
        if (register_addr == 31)
        {
            ushort dest = (ushort)((registers[18] << 8) + registers[19]);
            data = vdc_ram[dest++];
            registers[18] = (byte)(dest >> 8);
            registers[19] = (byte)dest;
        }

        return data;
    }
    else
    {
        ready = true;
        return 0xFF;
    }
}

void VDC8563::SetDataRegister(byte value)
{
    ready = false; // simulate delay in processing

    if (register_addr < registers_size)
    {
        registers[register_addr] = value;

        if (register_addr == 31)
        {
            ushort dest = (ushort)((registers[18] << 8) + registers[19]);
            vdc_ram[dest++] = value;
            registers[18] = (byte)(dest >> 8);
            registers[19] = (byte)dest;
        }
        else if (register_addr == 30)
        {
            int count = (value == 0) ? 256 : value;
            ushort dest = (ushort)((registers[18] << 8) + registers[19]);
            if ((registers[24] & 0x80) == 0)
            {
                for (int i = 0; i < count; ++i)
                    vdc_ram[dest++] = registers[31];
            }
            else
            {
                ushort src = (ushort)((registers[32] << 8) + registers[33]);
                for (int i = 0; i < count; ++i)
                    vdc_ram[dest++] = vdc_ram[src++];
                registers[32] = (byte)(src >> 8);
                registers[33] = (byte)src;
            }
            registers[18] = (byte)(dest >> 8);
            registers[19] = (byte)dest;
        }
    }
}
