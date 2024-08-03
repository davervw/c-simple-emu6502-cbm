// emuvic20.cpp - Class EmuVIC20 - Commodore VIC-20 Emulator
//
////////////////////////////////////////////////////////////////////////////////
//
// simple-emu-c64
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////
//
// This is a 6502 Emulator, designed for running Commodore 64 text mode, 
//   with only a few hooks: CHRIN-$FFCF/CHROUT-$FFD2/COLOR-$D021/199/646
// Useful as is in current state as a simple 6502 emulator
//
// LIMITATIONS:
// Only keyboard/console I/O.  No text pokes, no graphics.  Just stdio.  
//   No asynchronous input (GET K$), but INPUT S$ works
// No keyboard color switching.  No border displayed.  No border color.
// No screen editing (gasp!) Just short and sweet for running C64 BASIC in 
//   terminal/console window via 6502 chip emulation in software
// No PETSCII graphic characters, only supports printables CHR$(32) to CHR$(126), plus CHR$(147) clear screen
// No memory management.  Not full 64K RAM despite startup screen.
//   4K to 39K RAM (28K max for BASIC), 16K ROM, 1K VIC-II color RAM nybbles
// No timers.  No interrupts except BRK.  No NMI/RESTORE key.  No STOP key.
// No loading of files implemented.
//
// MEMORY MAP:
//   $0000-$03FF Low 1K RAM
//   $0400-$0FFF (3K RAM expansion)
//   $1000-$1DFF 3.5K RAM (for BASIC) [or alternate screen address]
//   $1E00-$1FFF 0.5K RAM (Screen characters)
//   $2000-$7FFF (24K RAM expansion)
//   $8000-$8FFF (Character ROM)
//   $9000-$9FFF (I/O)
//   $A000-$BFFF (8K Cartridge ROM, or RAM expansion)
//   $C000-$DFFF BASIC ROM
//   $E000-$FFFF KERNAL ROM
//
// Requires user provided Commodore Vic-20 BASIC/KERNAL ROMs (e.g. from VICE)
//   as they are not provided, others copyrights may still be in effect.
//
////////////////////////////////////////////////////////////////////////////////

#include "emuvic20.h"
#include "vic.h"
#include "config.h"
#include "CBMkeyboard.h"

// externs/globals
extern const char* StartupPRG;
extern int main_go_num;
extern int main_go_arg;

EmuVic20::EmuVic20(int ram_size) : EmuCBM(new Vic20Memory(ram_size * 1024))
{
  CBMkeyboard::reset(CBMkeyboard::Model::VIC20);
}

EmuVic20::~EmuVic20()
{
	delete memory;
	CBMkeyboard::waitKeysReleased(CBMkeyboard::Model::VIC20);
}

static byte RamSizeToRamConfig(int ram_size)
{
	if (ram_size < 8 * 1024)
		return 0x00; // 5K = 1K LOW + 4K BASE
	else if (ram_size < 13 * 1024)
		return 0x01; // 8K = 1K LOW + 3K EXP + 4K BASE (***ONLY*** CFG WHERE EXTRA 3K IS AVAILABLE TO BASIC)
	else if (ram_size < 16 * 1024)
		return 0x02; // 13K = 1K LOW + 4K BASE + 8K EXP
	else if (ram_size < 21 * 1024)
		return 0x03; // 16K = 1K LOW + 3K EXP + 4K BASE + 8K EXP (3K NOT AVAILABLE TO BASIC)
	else if (ram_size < 24 * 1024)
		return 0x06; // 21K = 1K LOW + 4K BASE + 16K EXP
	else if (ram_size < 29 * 1024)
		return 0x07; // 24K = 1K LOW + 3K EXP + 4K BASE + 16K EXP (3K NOT AVAILABLE TO BASIC)
	else if (ram_size < 32 * 1024)
		return 0x0E; // 29K = 1K LOW + 4K BASE + 24K EXP
	else if (ram_size < 37 * 1024)
		return 0x0F; // 32K = 1K LOW + 3K EXP + 4K BASE + 24K EXP (3K NOT AVAILABLE TO BASIC)
	else if (ram_size < 40 * 1024)
		return 0x1E; // 37K = 1K LOW + 4K BASE + 32K EXP (11K NOT AVAILABLE TO BASIC)
	else
		return 0x1F; // 40K = 1K LOW + 3K EXP + 4K BASE + 32K EXP (ALL NOT AVAILABLE TO BASIC)
}

static int go_state = 0;
static int startup_state = 0;

bool EmuVic20::ExecutePatch()
{
	static bool NMI = false;

	int found_NMI = 0;
	for (int i = 0; !found_NMI && i < 16; ++i)
		if (CBMkeyboard::scan_codes[i] & 1024)
			found_NMI = 1;

	if (NMI)
	{
		if (!found_NMI)
			NMI = false; // reset when not pressed
	}
	else if (found_NMI) // newly pressed, detected edge
	{
		NMI = true; // set so won't trigger again until cleared
		Push(HI(PC));
		Push(LO(PC));
		B = false; // only false on stack for NMI and IRQ
		PHP();
		B = true; // return to normal state
		PC = (ushort)(GetMemory(0xFFFA) + (GetMemory(0xFFFB) << 8)); // JMP(NMI)
		return true; // overriden, and PC changed, so caller should reloop before execution to allow breakpoint/trace/ExecutePatch/etc.
	}

#ifdef _WINDOWS
	static unsigned counter = 0;
	if ((++counter & 0x0400) == 0)
		((Vic20Memory*)memory)->vic->CheckPaintFrame(timer_now);
#endif

	if (PC == 0xC474 || PC == LOAD_TRAP) // READY
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
				FileAddr = (ushort)(GetMemory(43) | (GetMemory(44) << 8));
				is_basic = LoadStartupPrg();
				SetMemory(0xAE, (byte)FileAddr);
				SetMemory(0xAF, (byte)(FileAddr >> 8));
			}

			StartupPRG = 0;

			if (is_basic)
			{
				// initialize first couple bytes (may only be necessary for UNNEW?)
				ushort addr = (ushort)(GetMemory(43) | (GetMemory(44) << 8));
				SetMemory(addr, 1);
				SetMemory((ushort)(addr + 1), 1);

				startup_state = 1; // should be able to regain control when returns...

				return ExecuteJSR(0xC533); // LINKPRG
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
			ushort addr = (ushort)((GetMemory(0x22) | (GetMemory(0x23) << 8)) + 2);
			SetMemory(45, (byte)addr);
			SetMemory(46, (byte)(addr >> 8));

			SetA(0);

			startup_state = 2; // should be able to regain control when returns...

			return ExecuteJSR(0xC65E); // CLEAR/CLR
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
				SetMemory(198, 4);
				SetMemory(631, 'R');
				SetMemory(632, 'U');
				SetMemory(633, 'N');
				SetMemory(634, '\r');
				PC = 0xA47B; // skip READY message, but still set direct mode, and continue to MAIN
			}
			C = false; // signal success
			startup_state = 0;
			LOAD_TRAP = -1;
			return true; // overriden, and PC changed, so caller should reloop before execution to allow breakpoint/trace/ExecutePatch/etc.
		}
	}
	else if (PC == 0xC815) // Execute after GO
	{
		if (go_state == 0 && A >= (byte)'0' && A <= (byte)'9')
		{
			go_state = 1;
			return ExecuteJSR(0xCD8A); // Evaluate expression, check data type
		}
		else if (go_state == 1)
		{
			go_state = 2;
			return ExecuteJSR(0xD7F7); // Convert fp to 2 byte integer
		}
		else if (go_state == 2)
		{
			main_go_arg = 0;
			main_go_num = (ushort)(Y + (A << 8));
			if (main_go_num == 20 && memory->read(memory->read(0x7A) | (memory->read(0x7B) << 8)) == ',') {
				go_state = 3;
				A = ',';
				return ExecuteJSR(0xCEFD); // Validate comma
			}
			quit = true;
			return true;
		}
		else if (go_state == 3)
		{
			trace = false;
			go_state = 4;
			return ExecuteJSR(0xCD8A); // Evaluate expression, check data type
		}
		else if (go_state == 4)
		{
			go_state = 5;
			return ExecuteJSR(0xD7F7); // Convert fp to 2 byte integer
		}
		else if (go_state == 5)
		{
			main_go_arg = (ushort)(Y + (A << 8));
			quit = true;
			return true;
		}
	}

	if (main_go_num == 1) {
		quit = true;
		return true;
	}

	return EmuCBM::ExecutePatch();
}

//byte[] ram;
//// ram_lo;         // 1K: 0000-03FF
//// ram_3k;         // 3K: 0400-0FFF (bank0, Optional)
//// ram_default;    // 4K: 1000-1FFF (Always present, default video is 1E00-1FFF)
//// ram_8k1;        // 8K: 2000-3FFF (bank1, Optional)
//// ram_8k2;        // 8K: 4000-5FFF (bank2, Optional)
//// ram_8k3;        // 8K: 6000-7FFF (bank3, Optional)
//byte[] char_rom;   // 4K: 8000-8FFF
//byte[] io;         // 4K: 9000-9FFF (including video ram)
//// ram_rom         // 8K: A000-BFFF (bank4, Optional, Cartridge)
//byte[] basic_rom;  // 8K: C000-DFFF
//byte[] kernal_rom; // 8K: E000-FFFF

static const int ram3k_addr = 0x0400;
static const int ram4k_addr = 0x1000;
static const int ram8k1_addr = 0x2000;
static const int ram8k2_addr = 0x4000;
static const int ram8k3_addr = 0x6000;
static const int char_addr = 0x8000;
static const int char_size = 0x1000;
static const int io_addr = 0x9000;
static const int io_size = 0x1000;
static const int cart_addr = 0xA000;
static const int basic_addr = 0xC000;
static const int basic_size = 0x2000;
static const int kernal_addr = 0xE000;
static const int kernal_size = 0x2000;

EmuVic20::Vic20Memory::Vic20Memory(int size)
{
	ram_banks = RamSizeToRamConfig(size);

	// allocate max RAM size based on highest RAM address even though all memory may not be present
	ram_size = 0xC000;
	ram = new byte[ram_size];
	for (int i = 0; i < ram_size; ++i)
		ram[i] = 0;

	io = new byte[io_size];
	for (int i = 0; i < io_size; ++i)
		io[i] = 0;

	char_rom = new byte[char_size];
	basic_rom = new byte[basic_size];
	kernal_rom = new byte[kernal_size];

	EmuCBM::File_ReadAllBytes(char_rom, char_size, "/roms/vic20/chargen");
	EmuCBM::File_ReadAllBytes(basic_rom, basic_size, "/roms/vic20/basic");
	EmuCBM::File_ReadAllBytes(kernal_rom, kernal_size, "/roms/vic20/kernal");

	// hack rom to load filename "*" instead of empty filename when SHIFT+RUN
	kernal_rom[0xdf5] = 207; // shift O
	kernal_rom[0xdf6] = 34;  // double quote
	kernal_rom[0xdf7] = 42;  // asterisk

	kernal_rom[0x1d9] = 8; // default to floppy disk instead of tape

	vic = new EmuVic(ram, io, char_rom);
}

EmuVic20::Vic20Memory::~Vic20Memory()
{
	delete[] ram;
	delete[] io;
	delete[] char_rom;
	delete[] basic_rom;
	delete[] kernal_rom;
	delete vic;
}

byte EmuVic20::Vic20Memory::read(ushort addr)
{
	if (addr < ram3k_addr)
		return ram[addr];
	else if (addr >= ram3k_addr && addr < ram4k_addr && ((ram_banks & 0x01) != 0))
		return ram[addr];
	else if (addr >= ram4k_addr && addr < ram8k1_addr)
		return ram[addr];
	else if (addr >= ram8k1_addr && addr < ram8k2_addr && ((ram_banks & 0x02) != 0))
		return ram[addr];
	else if (addr >= ram8k2_addr && addr < ram8k3_addr && ((ram_banks & 0x04) != 0))
		return ram[addr];
	else if (addr >= ram8k3_addr && addr < char_addr && ((ram_banks & 0x08) != 0))
		return ram[addr];
	else if (addr >= char_addr && addr < char_addr + char_size)
		return char_rom[addr - char_addr];
	else if (addr >= io_addr && addr < io_addr + io_size)
	{
		if (addr == 0x912F)
			return 0xFF;
		else if (addr == 0x911C)
			return 0xFE;
		else if (addr == 0x911F)
			return 0x7E;
		else if (addr == 0x9121)
		{
			CBMkeyboard::ReadKeyboard(CBMkeyboard::Model::VIC20);

			if (CBMkeyboard::heldToggle)
				main_go_num = 1;

			byte value = 0xFF;
			for (int i = 0; i < 16; ++i)
			{
				int scan_code = CBMkeyboard::scan_codes[i] & 127; // remove any modifiers
				if (scan_code < 64)
				{
					int row = scan_code >> 3;
					int col = scan_code & 7;

					if ((io[0x9120 - io_addr] & (1 << row)) == 0)
						value &= ~(1 << col);
				}
			}
			return value;
		}
		return io[addr - io_addr];
	}
	else if (addr >= cart_addr && addr < basic_addr && ((ram_banks & 0x10) != 0))
		return ram[addr];
	else if (addr >= basic_addr && addr < basic_addr + basic_size)
		return basic_rom[addr - basic_addr];
	else if (addr >= kernal_addr && addr < kernal_addr + kernal_size)
		return kernal_rom[addr - kernal_addr];
	else
		return 0xFF;
}


void EmuVic20::Vic20Memory::write(ushort addr, byte value)
{
	if (addr < ram3k_addr)
	{
		ram[addr] = value;
		//if (addr == 199 || addr == 646)
		//    ApplyColor();
		//else if (addr == 216)
		//    CBM_Console.InsertMode = (value != 0);
		//else if (addr == 212)
		//    CBM_Console.QuoteMode = (value != 0);
	}
	else if (addr >= ram3k_addr && addr < ram4k_addr && ((ram_banks & 0x01) != 0))
		ram[addr] = value;
	else if (addr >= ram4k_addr && addr < ram8k1_addr) {
		ram[addr] = value;
		if (addr >= vic->video_ram_addr && addr < vic->video_ram_addr + 22 * 23) // TODO: check VIC IO for actual video memory address
			vic->DrawChar(addr - vic->video_ram_addr);
	}
	else if (addr >= ram8k1_addr && addr < ram8k2_addr && ((ram_banks & 0x02) != 0))
		ram[addr] = value;
	else if (addr >= ram8k2_addr && addr < ram8k3_addr && ((ram_banks & 0x04) != 0))
		ram[addr] = value;
	else if (addr >= ram8k3_addr && addr < char_addr && ((ram_banks & 0x08) != 0))
		ram[addr] = value;
	else if (addr >= io_addr && addr < io_addr + io_size)
	{
		if (addr == 0x9002)
		{
			if ((io[2] & 128) == (value & 128))
				return;
			io[addr - io_addr] = value;
			vic->video_ram_addr = (value & 128) ? 0x1e00 : 0x1000;
			vic->color_ram_addr = (value & 128) ? 0x9600 : 0x9400;
			vic->color_nybles = &vic->io[vic->color_ram_addr - 0x9000];
			vic->RedrawScreen();
			return;
		}
		if (io[addr - io_addr] == value)
			return; // optimize out no change
		io[addr - io_addr] = value;
		if (addr == 0x900F) // background/border/inverse
		{
			vic->DrawBorder(value);
			vic->RedrawScreen();
		}
		else if (addr == 0x9005) // includes graphics/lowercase switch
			vic->RedrawScreen();
		else if (addr >= vic->color_ram_addr && addr < vic->color_ram_addr + 22 * 23)
			vic->DrawChar(addr - vic->color_ram_addr);
	}
	else if (addr >= cart_addr && addr < basic_addr && ((ram_banks & 0x10) != 0))
		ram[addr] = value;
}
