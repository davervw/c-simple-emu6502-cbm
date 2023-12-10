// emupet.cpp - Class EmuPET - Commodore PET Emulator
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
// This is a 6502 Emulator, designed for running Commodore 64 text mode, 
//   with only a few hooks: CHRIN-$FFCF/CHROUT-$FFD2/COLOR-$D021/199/646
// Useful as is in current state as a simple 6502 emulator
//
// LIMITATIONS: See EmuC64
//
// PET 2001 (chicklet keyboard) MEMORY MAP:
//   $0000-$1FFF RAM (8k)
//   $0000-$3FFF RAM (16k)
//   $0000-$7FFF RAM (32k)
//   $8000-$8FFF Video RAM
//   $C000-$DFFF BASIC ROM (8K)
//   $E000-$E7FF Editor ROM (2K)
//   $E800-$EFFF I/O
//   $F000-$FFFF KERNAL ROM (4K)
//
// Requires user provided Commodore PET BASIC/KERNAL ROMs (e.g. from VICE)
//   as they are not provided, others copyrights may still be in effect.
//
// References (and cool links):
//   https://archive.org/details/COMPUTEs_Programming_the_PET-CBM_1982_Small_Systems_Services
//   http://www.6502.org/users/andre/petindex/roms.html
//   http://www.6502.org/users/andre/petindex/progmod.html
//   https://en.wikipedia.org/wiki/Commodore_BASIC
//   http://www.weihenstephan.org/~michaste/pagetable/wait6502/msbasic_timeline.pdf
//   https://archive.org/details/COMPUTEs_First_Book_of_PET-CBM_1981_Small_Systems_Services
//   
////////////////////////////////////////////////////////////////////////////////

#include "emupet.h"
#include "config.h"

// externs/globals
extern int main_go_num;
extern const char* StartupPRG;

// locals
static int startup_state = 0;
static int fg_color = 0x07E0;
static int bg_color = 0x0000;

EmuPET::EmuPET(int ram_size) : EmuCBM(new PETMemory(ram_size * 1024))
{
}

EmuPET::~EmuPET()
{
}

// The patches implemented below are for basic1.  WARNING: basic2 is very different including zero page memory usage (e.g. $28/$29 instead of $7A/$7B)
//    = INDEX, temporary BASIC pointer, set before CLR
//   $7A/7B = start of BASIC program in RAM
//   $7C/7D = end of BASIC program in RAM, start of variables
//   $C38B = ROM BASIC READY prompt
//   $C394 = ROM BASIC MAIN, in direct mode but skip READY prompt
//   $C433= ROM LNKPRG/LINKPRG
//   $C770 = CLEAR/CLR - erase variables (token 9C)
//   $C6EC = EXECUTE can catch G for GO (because GO is not a keyword in basic1 ROM)
//   execute RESET vector captured to clear screen via CHR$(147)
bool EmuPET::ExecutePatch()
{
	if (PC == 0xC38B || PC == LOAD_TRAP) // READY
	{
		//go_state = 0;

		if (LOAD_TRAP != -1) // User requested program be loaded
		{
			bool is_basic;
			if (PC == LOAD_TRAP)
			{
				is_basic = (
					FileVerify == false
					&& FileSec == 0 // relative load, not absolute
					);
				byte err;
				bool success = FileLoad(&err);
				if (!success)
				{
					//System.Diagnostics.Debug.WriteLine(string.Format("FileLoad() failed: err={0}, file {1}", err, StartupPRG ? ? FileName));
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
				FileAddr = (ushort)(GetMemory(0x7A) | (GetMemory(0x7B) << 8));
				is_basic = LoadStartupPrg();
			}

			StartupPRG = 0;

			if (is_basic)
			{
				// initialize first couple bytes (may only be necessary for UNNEW?)
				ushort addr = (ushort)(GetMemory(0x7A) | (GetMemory(0x7B) << 8));
				SetMemory(addr, 1);
				SetMemory((ushort)(addr + 1), 1);

				startup_state = 1; // should be able to regain control when returns...

				return ExecuteJSR(0xC430); // Reset BASIC execution to start and fall through to LINKPRG - PET 2001 Basic 1.0
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
			ushort addr = (ushort)(GetMemory(0x71) | (GetMemory(0x72) << 8) + 2);
			SetMemory(0x7C, (byte)addr);
			SetMemory(0x7D, (byte)(addr >> 8));

			SetA(0);

			startup_state = 2; // should be able to regain control when returns...

			return ExecuteJSR(0xC56A); // CLEAR/CLR
		}
		else if (startup_state == 2)
		{
			if (PC == LOAD_TRAP)
			{
				X = LO(FileAddr);
				Y = HI(FileAddr);

				PC = 0xC38B; // READY
			}
			else
			{
				//CBM_Console.Push("RUN\r");
				PC = 0xC394; // skip READY message, but still set direct mode, and continue to MAIN
			}
			C = false; // signal success
			startup_state = 0;
			LOAD_TRAP = -1;
			return true; // overriden, and PC changed, so caller should reloop before execution to allow breakpoint/trace/ExecutePatch/etc.
		}
	}
	else if (PC == 0xC6EC && A == 'G') // EXECUTE and found G where token expected
	{
		ushort addr = (ushort)(GetMemory(0xC9) | (GetMemory(0xCA) << 8));
		if (GetMemory((ushort)(++addr)) == 'O')
		{
			// skip optional space
			if (GetMemory((ushort)(++addr)) == ' ')
				++addr;

			int digits = 0;
			ushort num = 0;
			while (true)
			{
				char c = (char)GetMemory(addr++);
				if (digits > 0 && (c == 0 || c == ':')) // end of number?
				{
					main_go_num = num;
					quit = true;
					return true;
				}
				else if (c >= '0' && c <= '9')
				{
					++digits;
					num = (num * 10 + (c - '0')); // append digit, conversion from ASCII to ushort
				}
				else
					return false;
			}
		}
	}
	else if (PC == 0xF34E) // LOAD (after arguments parsed)
	{
		// PET is different, get arguments from low memory in fixed places
		FileAddr = (ushort)(GetMemory(0xe5) | (GetMemory(0xe6) << 8));
		FileVerify = (GetMemory(0x20B) == 1);
		static char name[256];
		int len = GetMemory(0xEE);
		ushort fn_index = (ushort)(GetMemory(0xF9) | (GetMemory(0xFA) << 8));
		for (int i = 0; i < len; ++i)
			name[i] = (char)GetMemory((ushort)(fn_index + i));
		name[len] = 0;
		StartupPRG = &name[0];

		FileSec = GetMemory(0xF0);
		FileDev = GetMemory(0xF1);

		const char* op;
		switch (GetMemory(0x20B))
		{
		case 0: op = "LOAD"; break;
		case 1: op = "VERIFY"; break;
		default: op = "???"; break;
		}
		//System.Diagnostics.Debug.WriteLine(string.Format("{0} filename={1} device={2} sec={3} at={4:X4}", op, filename, FileDev, FileSec, FileAddr));

		if (FileSec == 0)
			FileAddr = (ushort)(GetMemory(0x7A) | (GetMemory(0x7B) << 8)); // fix: PET erroneously sets address to $0100 instead of $0400 for relative

		//System.Diagnostics.Debug.WriteLine(string.Format("{0} filename={1} device={2} sec={3} at={4:X4}", op, filename, FileDev, FileSec, FileAddr));

		ExecuteRTS();

		if (op != "???")
			LOAD_TRAP = PC;

		return true; // overriden, and PC changed, so caller should reloop before execution to allow breakpoint/trace/ExecutePatch/etc.
	}
	else if (PC == 0xFFD8) // SAVE
		;
	else if ( // PET is different, so don't trap these addresses
		PC == 0xFFBA // SETLFS
		|| PC == 0xFFBD // SETNAM
		)
		return false;

	// if got here, then call base class EmuCBM.ExecutePatch() for common code
	return EmuCBM::ExecutePatch();
}

const int video_addr = 0x8000;
const int video_size = 0x1000;
const int basic_addr = 0xC000;
const int basic_size = 0x2000;
const int edit_addr = 0xE000;
const int edit_size = 0x0800;
const int io_addr = 0xE800;
const int io_size = 0x0800;
const int kernal_addr = 0xF000;
const int kernal_size = 0x1000;
const int chargen_size = 0x0800;

EmuPET::PETMemory::PETMemory(int size)
{
	ram_size = size;
	if (ram_size > 32 * 1024) // too  big?
		ram_size = 32 * 1024; // truncate
	ram = new byte[ram_size];
	for (int i = 0; i < ram_size; ++i)
		ram[i] = 0;

	video_ram = new byte[video_size];
	for (int i = 0; i < video_size; ++i)
		video_ram[i] = 0;

	io = new byte[io_size];
	for (int i = 0; i < io_size; ++i)
		io[i] = 0;

	basic = new byte[basic_size];
	EmuCBM::File_ReadAllBytes(basic, basic_size, "/roms/pet/basic1");

	edit = new byte[edit_size];
	EmuCBM::File_ReadAllBytes(edit, edit_size, "/roms/pet/edit1g");

	kernal = new byte[kernal_size];
	EmuCBM::File_ReadAllBytes(kernal, kernal_size, "/roms/pet/kernal1");

  video = new PETVideo(video_ram); 
}

EmuPET::PETMemory::~PETMemory()
{
	delete[] ram;
	delete[] video_ram;
	delete[] io;
	delete[] basic;
	delete[] edit;
	delete[] kernal;
  delete video;
}

byte EmuPET::PETMemory::read(ushort addr)
{
	if (addr < ram_size)
		return ram[addr];
	else if (addr >= video_addr && addr < video_addr + video_size)
		return video_ram[addr - video_addr];
	else if (addr >= basic_addr && addr < basic_addr + basic_size)
		return basic[addr - basic_addr];
	else if (addr >= edit_addr && addr < edit_addr + edit_size)
		return edit[addr - edit_addr];
	else if (addr == 0xE810) // PORT A
		return 0xFF; // return FF otherwise hangs on start, key scan?
	else if (addr >= io_addr && addr < io_addr + io_size)
		return io[addr - io_addr];
	else if (addr >= kernal_addr && addr < kernal_addr + kernal_size)
		return kernal[addr - kernal_addr];
	else
		return 0xFF;
}

void EmuPET::PETMemory::write(ushort addr, byte value)
{
	if (addr < ram_size)
	{
		ram[addr] = value;
	}
	else if (addr >= video_addr && addr < video_addr + video_size)
  {
    bool update_required = (video_ram[addr - video_addr] != value);
    if (update_required)
    {
		  video_ram[addr - video_addr] = value;
      video->DrawChar(addr - video_addr);
    }
  }
	else if (addr >= io_addr && addr < io_addr + io_size)
		io[addr - io_addr] = value;
}

EmuPET::PETVideo::PETVideo(byte* vram)
{
  video_ram = vram;
	chargen = new byte[chargen_size];
	EmuCBM::File_ReadAllBytes(chargen, chargen_size, "/roms/pet/characters2.bin");

  // initialize LCD screen
  M5.Lcd.fillScreen(0x0000);  // BLACK
  DrawBorder();
}

EmuPET::PETVideo::~PETVideo()
{
  delete[] chargen;
}

void EmuPET::PETVideo::DrawChar(byte c, int col, int row, int fg, int bg)
{
  int offset = 0; // no lowercase on 2001
  const byte* shape = &chargen[c*8+offset];
  int x0 = 0 + col*8;
  int y0 = 20 + row*8;
  for (int row_i=0; row_i<8; ++row_i)
  {
    int mask = 128;
    for (int col_i=0; col_i<8; ++col_i)
    {
      M5.Lcd.drawPixel(x0+col_i, y0+row_i, ((shape[row_i] & mask) == 0) ? bg : fg);
      mask = mask >> 1;
    }
  }
}

void EmuPET::PETVideo::DrawChar(int offset)
{
  int col = offset % 40;
  int row = offset / 40;
  int fg = fg_color;
  int bg = bg_color;
  DrawChar(video_ram[offset], col, row, fg, bg);
}

void EmuPET::PETVideo::RedrawScreen()
{
  int offset = 0;
  for (int row = 0; row < 25; ++row)
  {
    for (int col = 0; col < 40; ++col)
    {
      DrawChar(video_ram[offset], col, row, fg_color, bg_color);
      ++offset;
    }
  }
}

void EmuPET::PETVideo::DrawBorder()
{
    int color = bg_color;
    M5.Lcd.fillRect(0, 0, 320, 20, color);
    M5.Lcd.fillRect(0, 220, 320, 20, color);
}
