// emuc64.c - Commodore 64 Emulator
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C Portable Version)
// C64/6502 Emulator for Microsoft Windows Console
//
// MIT License
//
// Copyright(c) 2020 by David R.Van Wagner
// davevw.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//
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
// No PETSCII graphic characters, only supports printables CHR$(32) to CHR$(126), and CHR$(147) clear screen
// No memory management.  Full 64K RAM not accessible via banking despite startup screen.
//   Just 44K RAM, 16K ROM, 1K VIC-II color RAM nybbles
// No timers.  No interrupts except BRK.  No NMI/RESTORE key.  No STOP key.
// No loading of files implemented.
//
//   $00/$01     (DDR and banking and I/O of 6510 missing), just RAM
//   $0000-$9FFF RAM (199=reverse if non-zero, 646=foreground color)
//   $A000-$BFFF BASIC ROM (write to RAM underneath, but haven't implemented read/banking)
//   $C000-$CFFF RAM
//   $D000-$DFFF (missing I/O and character ROM and RAM banks), just zeros except...
//   $D021       Background Screen Color
//   $D800-$DFFF VIC-II color RAM nybbles (note: haven't implemented RAM banking)
//   $E000-$FFFF KERNAL ROM (write to RAM underneath, but haven't implemented read/banking)
//
// Requires user provided Commodore 64 BASIC/KERNAL ROMs (e.g. from VICE)
//   as they are not provided, others copyrights may still be in effect.
//
////////////////////////////////////////////////////////////////////////////////

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//uncomment for Commodore foreground, background colors and reverse emulation
//#define CBM_COLOR
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

#include "emu6502.h"
#include "cbmconsole.h"
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#ifdef WIN32
#include <share.h>
#else
#include <errno.h>
#include <unistd.h>
#endif

char* StartupPRG = 0;
int startup_state = 0;

static void File_ReadAllBytes(byte* bytes, unsigned int size, const char* filename)
{
	int file;
#ifdef WIN32	
	_set_errno(0);
	_sopen_s(&file, filename, _O_RDONLY | _O_BINARY, _SH_DENYNO, _S_IREAD);
#else
	file = open(filename, O_RDONLY);
#endif	
	if (file < 0)
	{
		char buffer[40];
#ifdef WIN32
		strerror_s(buffer, sizeof(buffer), errno);
#else		
		strerror_r(errno, buffer, sizeof(buffer));
#endif		
		printf("file ""%""s, errno=%d, %s", filename, errno, buffer);
		exit(1);
	}
#ifdef WIN32
	_read(file, bytes, size);
	_close(file);
#else
	read(file, bytes, size);
	close(file);
#endif	
}

// returns true if BASIC
static bool LoadPRG(const char* filename)
{
	bool result;
	byte lo, hi;
	int file;
	ushort loadaddr;
	
#ifdef WIN32	
	_set_errno(0);
	_sopen_s(&file, filename, _O_RDONLY | _O_BINARY, _SH_DENYNO, _S_IREAD);
#else
	file = open(filename, O_RDONLY);
#endif	
	if (file < 0
#ifdef WIN32
		|| _read(file, &lo, 1) != 1
		|| _read(file, &hi, 1) != 1
#else
		|| read(file, &lo, 1) != 1
		|| read(file, &hi, 1) != 1
#endif		
		)
	{
		char buffer[40];
#ifdef WIN32
		strerror_s(buffer, sizeof(buffer), errno);
#else
		strerror_r(errno, buffer, sizeof(buffer));
#endif		
		printf("file ""%""s, errno=%d, %s", filename, errno, buffer);
		exit(1);
	}
	if (lo == 1)
	{
		loadaddr = (ushort)(GetMemory(43) | (GetMemory(44) << 8));
		result = true;
	}
	else
	{
		loadaddr = (ushort)(lo | (hi << 8));
		result = false;
	}
	while (true)
	{
		byte value;
#ifdef WIN32
		if (_read(file, &value, 1) == 1)
#else
		if (read(file, &value, 1) == 1)
#endif		
			SetMemory(loadaddr++, value);
		else
			break;
	}
#ifdef WIN32
	_close(file);
#else
	close(file);
#endif	
	return result;
}

extern bool ExecutePatch(void)
{
	if (PC == 0xFFD2) // CHROUT
	{
		CBM_Console_WriteChar((char)A);
		// fall through to draw character in screen memory too
	}
	else if (PC == 0xFFCF) // CHRIN
	{
		A = CBM_Console_ReadChar();

		// SetA equivalent for flags
		Z = (A == 0);
		N = ((A & 0x80) != 0);
		C = false;

		// RTS equivalent
		byte lo = Pop();
		byte hi = Pop();
		PC = (ushort)(((hi << 8) | lo) + 1);

		return true; // overriden, and PC changed, so caller should reloop before execution to allow breakpoint/trace/ExecutePatch/etc.
	}
	else if (PC == 0xA474) // READY
	{
		if (StartupPRG != 0 && strlen(StartupPRG) > 0) // User requested program be loaded at startup
		{
			const char* filename = StartupPRG;
			StartupPRG = 0;

			if (LoadPRG(filename))
			{
				//UNNEW that I used in late 1980s, should work well for loang a program too, probably gleaned from BASIC ROM
				//ldy #0
				//lda #1
				//sta(43),y
				//iny
				//sta(43),y
				//jsr $a533 ; LINKPRG
				//clc
				//lda $22
				//adc #2
				//sta 45
				//lda $23
				//adc #0
				//sta 46
				//lda #0
				//jsr $a65e ; CLEAR/CLR
				//jmp $a474 ; READY

				// This part shouldn't be necessary as we have loaded, not recovering from NEW, bytes should still be there
				ushort addr = (ushort)(GetMemory(43) | (GetMemory(44) << 8));
				SetMemory(addr, 1);
				SetMemory((ushort)(addr + 1), 1);

				// JSR equivalent
				ushort retaddr = (ushort)(PC - 1);
				Push(HI(retaddr));
				Push(LO(retaddr));
				PC = 0xA533; // LINKPRG

				startup_state = 1; // should be able to regain control when returns...

				return true; // overriden, and PC changed, so caller should reloop before execution to allow breakpoint/trace/ExecutePatch/etc.
			}
		}
		else if (startup_state == 1)
		{
			ushort addr = (ushort)(GetMemory(0x22) | (GetMemory(0x23) << 8) + 2);
			SetMemory(45, (byte)addr);
			SetMemory(46, (byte)(addr >> 8));

			// JSR equivalent
			ushort retaddr = (ushort)(PC - 1);
			Push(HI(retaddr));
			Push(LO(retaddr));
			PC = 0xA65E; // CLEAR/CLR
			A = 0;

			startup_state = 2; // should be able to regain control when returns...

			return true; // overriden, and PC changed, so caller should reloop before execution to allow breakpoint/trace/ExecutePatch/etc.
		}
		else if (startup_state == 2)
		{
			CBM_Console_Push("RUN\r");
			PC = 0xA47B; // skip READY message, but still set direct mode, and continue to MAIN
			C = false;
			startup_state = 0;
			return true; // overriden, and PC changed, so caller should reloop before execution to allow breakpoint/trace/ExecutePatch/etc.
		}
	}
	return false; // execute normally
}

static byte ram[64 * 1024];
static byte basic_rom[8 * 1024];
static byte kernal_rom[8 * 1024];
static byte color_nybles[1024];

// note ram starts at 0x0000
const int basic_addr = 0xA000;
const int kernal_addr = 0xE000;
const int io_addr = 0xD000;
const int io_size = 0x1000;
const int color_addr = 0xD800;
const int open_addr = 0xC000;
const int open_size = 0x1000;

extern void C64_Init(int ram_size, const char* basic_file, const char* kernal_file)
{
	File_ReadAllBytes(basic_rom, sizeof(basic_rom), basic_file);
	File_ReadAllBytes(kernal_rom, sizeof(kernal_rom), kernal_file);

	for (int i = 0; i < sizeof(ram); ++i)
		ram[i] = 0;
	for (int i = 0; i < sizeof(color_nybles); ++i)
		color_nybles[i] = 0;

	//io = new byte[io_size];
	//for (int i = 0; i < io.Length; ++i)
	//	io[i] = 0;
}

extern byte GetMemory(ushort addr)
{
	if (addr < sizeof(ram) && (addr < basic_addr || (addr >= open_addr && addr < open_addr + open_size)))
		return ram[addr];
	else if (addr >= basic_addr && addr < basic_addr + sizeof(basic_rom))
		return basic_rom[addr - basic_addr];
	else if (addr >= color_addr && addr < color_addr + sizeof(color_nybles))
		return color_nybles[addr - color_addr];
	else if (addr >= io_addr && addr < io_addr + io_size)
		return 0; // io[addr - io_addr];
	else if (addr >= kernal_addr && addr < kernal_addr + sizeof(kernal_rom))
		return kernal_rom[addr - kernal_addr];
	else
		return 0xFF;
}

extern void SetMemory(ushort addr, byte value)
{
	if (addr < sizeof(ram) && (addr < io_addr || (addr >= kernal_addr && addr < kernal_addr + sizeof(kernal_rom))))
		ram[addr] = value;
	else if (addr == 0xD021) // background
		;
	else if (addr >= color_addr && addr < color_addr + sizeof(color_nybles))
		color_nybles[addr - color_addr] = value;
	//else if (addr >= io_addr && addr < io_addr + io.Length)
	//    io[addr - io_addr] = value;
}
