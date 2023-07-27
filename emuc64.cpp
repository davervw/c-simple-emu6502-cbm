// emuc64.cpp - Commodore 64 Emulator
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
#include "emud64.h"

#include "cbmconsole.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#ifdef WIN32
#include <io.h>
#include <share.h>
#include <windows.h> // struct timeval
extern "C" int gettimeofday(struct timeval* tp, struct timezone* tzp);
#else
#include <errno.h>
#include <unistd.h>
#include <sys/time.h> // gettimeofday, struct timeval
#endif

extern "C" void* D64_CreateOrLoad(const char* filename);
extern "C" int D64_GetDirectoryProgram(void* disk, unsigned char* buffer, int* p_ret_file_len);
extern "C" void D64_ReadFileByName(void* disk, unsigned char* filename, unsigned char* buffer, int* p_ret_file_len);
extern "C" int D64_FileSave(void* disk, char* filename, unsigned char* buffer, int buffer_len);

extern const char* StartupPRG;
int startup_state = 0;
int LOAD_TRAP = -1;
byte* attach = NULL;
void* disk = NULL;

#include "emuc64.h"

EmuC64::EmuC64(int ram_size,
	const char* basic_file,
	const char* chargen_file,
	const char* kernal_file)
	: EmuCBM(new C64Memory(ram_size,
		basic_file,
		chargen_file,
		kernal_file))
{
	FileName = NULL;
	FileNum = 0;
	FileDev = 0;
	FileSec = 0;
	FileVerify = false;
	FileAddr = 0;
}

EmuC64::~EmuC64()
{
	delete memory;
}

byte EmuC64::GetMemory(ushort addr)
{
	return memory->read(addr);
}

void EmuC64::SetMemory(ushort addr, byte value)
{
	memory->write(addr, value);
}

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
bool EmuCBM::LoadPRG(const char* filename)
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

bool EmuC64::ExecuteRTS()
{
	byte bytes;
	RTS(&PC, &bytes);
	return true; // return value for ExecutePatch so will reloop execution to allow berakpoint/trace/ExecutePatch/etc.
}

bool EmuC64::ExecuteJSR(ushort addr)
{
	ushort retaddr = (PC - 1) & 0xFFFF;
	Push(HI(retaddr));
	Push(LO(retaddr));
	PC = addr;
	return true; // return value for ExecutePatch so will reloop execution to allow berakpoint/trace/ExecutePatch/etc.
}

static byte* OpenRead(const char* filename, int* p_ret_file_len)
{
	static unsigned char buffer[65536]; // TODO: get actual file size

	if (disk == 0)
	{
		return (byte*)NULL;
		*p_ret_file_len = 0;
	}

	if (filename != NULL && filename[0] == '$' && filename[1] == '\0')
	{
		*p_ret_file_len = sizeof(buffer);
		if (D64_GetDirectoryProgram(disk, buffer, p_ret_file_len))
			return &buffer[0];
		else
		{
			return (byte*)NULL;
			*p_ret_file_len = 0;
		}
	}
	else
	{
		*p_ret_file_len = sizeof(buffer);
		D64_ReadFileByName(disk, (unsigned char*)filename, buffer, p_ret_file_len);
		return buffer;
	}
}

// returns success
bool EmuC64::FileLoad(byte* p_err)
{
	bool startup = (StartupPRG != NULL);
	ushort addr = FileAddr;
	bool success = true;
	byte err = 0;
	const char* filename = (StartupPRG != NULL) ? StartupPRG : FileName;
	int file_len;
	byte* bytes = OpenRead(filename, &file_len);
	ushort si = 0;
	if (bytes == 0 || file_len == 0) {
		*p_err = 4; // FILE NOT FOUND
		success = false;
		FileAddr = addr;
		return success;
	}

	byte lo = bytes[si++];
	byte hi = bytes[si++];
	if (startup) {
		if (lo == 1)
			FileSec = 0;
		else
			FileSec = 1;
	}
	if (FileSec == 1) // use address in file? yes-use, no-ignore
		addr = lo | (hi << 8); // use address specified in file
	while (success) {
		if (si < file_len) {
			byte i = bytes[si++];
			if (FileVerify) {
				if (GetMemory(addr) != i) {
					*p_err = 28; // VERIFY
					success = false;
				}
			}
			else
				SetMemory(addr, i);
			++addr;
		}
		else
			break; // end of file
	}
	FileAddr = addr;
	return success;
}

bool EmuC64::FileSave(const char* filename, ushort addr1, ushort addr2)
{
	if (filename == NULL || *filename == 0)
		filename = "FILENAME";
	if (disk == 0)
		return false;
	int len = addr2 - addr1 + 2;
	unsigned char* bytes = (unsigned char*)malloc(len);
	if (bytes == 0 || len < 2)
		return false;
	bytes[0] = LO(addr1);
	bytes[1] = HI(addr1);
	for (int i = 0; i < len-2; ++i)
		bytes[i+2] = GetMemory(addr1+i);
	int result = D64_FileSave(disk, (char*)filename, bytes, len);
	free(bytes);
	return result;
}

bool EmuC64::LoadStartupPrg()
{
	bool result;
	byte err;
	if (disk == 0)
		disk = D64_CreateOrLoad(FileName);
	result = FileLoad(&err);
	if (!result)
		return false;
	else
		return FileSec == 0 ? true : false; // relative is BASIC, absolute is ML
}

void EmuC64::CheckBypassSETNAM()
{
	// In case caller bypassed calling SETNAM, get from lower memory
	byte name_len = GetMemory(0xB7);
	ushort name_addr = (ushort)(GetMemory(0xBB) | (GetMemory(0xBC) << 8));
	static char name[256];
	memset(name, 0, sizeof(name));
	for (int i = 0; i < name_len; ++i)
		name[i] = GetMemory(name_addr + i);
	if (FileName != 0 || strlen(FileName) != strlen(name) || memcmp(FileName, name, strlen(name)) != 0)
		FileName = name;
}

void EmuC64::CheckBypassSETLFS()
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
	}
}

bool EmuC64::ExecutePatch()
{
	if (PC == 0xFFD2) // CHROUT
	{
		CBM_Console_WriteChar((char)A);
		// fall through to regular routine to draw character in screen memory too
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
	else if (PC == 0xA474 || PC == LOAD_TRAP) // READY
	{
		if (startup_state == 0 && ((StartupPRG != 0 && strlen(StartupPRG) > 0) || PC == LOAD_TRAP))
		{
			bool is_basic;
			if (PC == LOAD_TRAP) {
				is_basic = (
					FileVerify == false
					&& FileSec == 0 // relative load, not absolute
					&& LO(FileAddr) == GetMemory(43) // requested load address matches BASIC start
					&& HI(FileAddr) == GetMemory(44)
					);
				bool success;
				byte err;
				success = FileLoad(&err);
				if (success)
				{
					// set End of Program
					SetMemory(0xAE, (byte)FileAddr);
					SetMemory(0xAF, (byte)(FileAddr >> 8));
				}
				else
				{
					//console.log("FileLoad() failed: err=" + err + ", file " + StartupPRG);
					C = true; // signal error
					SetA(err); // FILE NOT FOUND or VERIFY

					// so doesn't repeat
					StartupPRG = "";
					LOAD_TRAP = -1;
					attach = NULL;

					return true; // overriden, and PC changed, so caller should reloop before execution to allow breakpoint/trace/ExecutePatch/etc.
				}
			}
			else {
				FileName = StartupPRG;
				FileAddr = GetMemory(43) | (GetMemory(44) << 8);
				is_basic = LoadStartupPrg();
			}

			StartupPRG = 0;
			attach = NULL;

			if (is_basic) {
				// UNNEW that I used in late 1980s, should work well for loading a program too, probably gleaned from BASIC ROM
				// listed here as reference, adapted to use in this state machine, ExecutePatch()
				// ldy #0
				// lda #1
				// sta(43),y
				// iny
				// sta(43),y
				// jsr $a533 ; LINKPRG
				// clc
				// lda $22
				// adc #2
				// sta 45
				// lda $23
				// adc #0
				// sta 46
				// lda #0
				// jsr $a65e ; CLEAR/CLR
				// jmp $a474 ; READY

				// This part shouldn't be necessary as we have loaded, not recovering from NEW, bytes should still be there
				// initialize first couple bytes (may only be necessary for UNNEW?)
				ushort addr = GetMemory(43) | (GetMemory(44) << 8);
				SetMemory(addr, 1);
				SetMemory((ushort)(addr + 1), 1);

				startup_state = 1; // should be able to regain control when returns...

				return ExecuteJSR(0xA533); // LINKPRG
			}
			else {
				LOAD_TRAP = -1;
				X = LO(FileAddr);
				Y = HI(FileAddr);
				C = false;
			}
		}
		else if (startup_state == 1) {
			ushort addr = GetMemory(0x22) + (GetMemory(0x23) << 8) + 2;
			SetMemory(45, LO(addr));
			SetMemory(46, HI(addr));

			SetA(0);

			startup_state = 2; // should be able to regain control when returns...

			return ExecuteJSR(0xA65E); // CLEAR/CLR
		}
		else if (startup_state == 2) {
			if (PC == LOAD_TRAP) {
				X = LO(FileAddr);
				Y = HI(FileAddr);
			}
			else {
				CBM_Console_Push("RUN\r");
				PC = 0xA47B; // skip READY message, but still set direct mode, and continue to MAIN
			}
			C = false; // signal success
			startup_state = 0;
			LOAD_TRAP = -1;
			return true; // overriden, and PC changed, so caller should reloop before execution to allow breakpoint/trace/ExecutePatch/etc.
		}
	}
	else if (PC == 0xFFBA) // SETLFS
	{
		FileNum = A;
		FileDev = X;
		FileSec = Y;
		//console.log("SETLFS " + FileNum + ", " + FileDev + ", " + FileSec);
	}
	else if (PC == 0xFFBD) // SETNAM
	{
		static char name[256];
		memset(name, 0, sizeof(name));
		ushort addr = X | (Y << 8);
		for (int i = 0; i < A; ++i)
			name[i] = GetMemory(addr + i);
		//console.log("SETNAM " + name);
		FileName = name;
	}
	else if (PC == 0xFFD5) // LOAD
	{
		FileAddr = X | (Y << 8);
		//let op : string;
		//if (A == 0)
		//	op = "LOAD";
		//else if (A == 1)
		//	op = "VERIFY";
		//else
		//	op = "LOAD (A=" + A + ") ???";
		FileVerify = (A == 1);
		//console.log(op + " @" + Emu6502.toHex16(FileAddr));

		ExecuteRTS();

		if (strlen(FileName) == 0)
		{
			SetA(8); // EMPTY filename message
			C = true; // failure
		}
		else if (A == 0 || A == 1) {
			LOAD_TRAP = PC;
			C = false; // success
		}
		else {
			FileName = 0;
			SetA(14); // ILLEGAL QUANTITY message
			C = true; // failure
		}

		return true; // overriden, and PC changed, so caller should reloop before execution to allow breakpoint/trace/ExecutePatch/etc.
	}
	else if (PC == 0xFFD8) // SAVE
	{
		ushort addr1 = GetMemory(A) | (GetMemory((A + 1) & 0xFF) << 8);
		ushort addr2 = X | (Y << 8);
		//console.log("SAVE " + Emu6502.toHex16(addr1) + "-" + Emu6502.toHex16(addr2));

		// Set success
		C = !FileSave(FileName, addr1, addr2);

		return ExecuteRTS();
	}
	if (GetMemory(PC) == 0x6C && GetMemory((ushort)(PC + 1)) == 0x30 && GetMemory((ushort)(PC + 2)) == 0x03) // catch JMP(LOAD_VECTOR), redirect to jump table
	{
		CheckBypassSETLFS();
		CheckBypassSETNAM();
		// note: A register has same purpose LOAD/VERIFY
		X = GetMemory(0xC3);
		Y = GetMemory(0xC4);
		PC = 0xFFD5; // use KERNAL JUMP TABLE instead, so LOAD is hooked by base
		return true; // re-execute
	}
	if (GetMemory(PC) == 0x6C && GetMemory((ushort)(PC + 1)) == 0x32 && GetMemory((ushort)(PC + 2)) == 0x03) // catch JMP(SAVE_VECTOR), redirect to jump table
	{
		CheckBypassSETLFS();
		CheckBypassSETNAM();
		X = GetMemory(0xAE);
		Y = GetMemory(0xAF);
		A = 0xC1;
		PC = 0xFFD8; // use KERNAL JUMP TABLE instead, so SAVE is hooked by base
		return true; // re-execute
	}
	// else if (PC == 0xa7e4 && !trace) // execute statement
	// {
	//     trace = true;
	//     return true; // call again, so traces this line
	// } 
	return false;
}

// note ram starts at 0x0000
const int basic_addr = 0xA000;
const int kernal_addr = 0xE000;
const int io_addr = 0xD000;
const int io_size = 0x1000;
const int color_addr = 0xD800;
const int open_addr = 0xC000;
const int open_size = 0x1000;
const int basic_rom_size = 8 * 1024;
const int char_rom_size = 4 * 1024;
const int kernal_rom_size = 8 * 1024;
const int color_nybles_size = 1024;

// C64Memory //////////////////////////////////////////////////////////////////

C64Memory::C64Memory(int memory_size,
	const char* basic_file,
	const char* chargen_file,
	const char* kernal_file
)
{
	ram_size = memory_size;
	ram = new byte[ram_size];
	basic_rom = new byte[basic_rom_size];
	char_rom = new byte[char_rom_size];
	kernal_rom = new byte[kernal_rom_size];
	color_nybles = new byte[color_nybles_size];

	File_ReadAllBytes(basic_rom, basic_rom_size, basic_file);
	File_ReadAllBytes(char_rom, char_rom_size, chargen_file);
	File_ReadAllBytes(kernal_rom, kernal_rom_size, kernal_file);

	for (int i = 0; i < ram_size; ++i)
		ram[i] = 0;
	for (int i = 0; i < color_nybles_size; ++i)
		color_nybles[i] = 0;

	//io = new byte[io_size];
	//for (int i = 0; i < io.Length; ++i)
	//	io[i] = 0;

	// initialize DDR and memory mapping to defaults
	ram[0] = 0xEF;
	ram[1] = 0x07;
}

C64Memory::~C64Memory()
{
	delete[] ram;
	delete[] basic_rom;
	delete[] kernal_rom;
	delete[] color_nybles;
}

byte C64Memory::read(ushort addr)
{
	if (addr == 0xa2 ) {
			// RDTIM
			time_t  now = time(0);
			struct tm       bd;
			struct timeval  tv;

#ifdef WIN32
			localtime_s(&bd, &now);
#else
			localtime_r(&now, &bd);
#endif
			gettimeofday(&tv, 0);

			unsigned long jiffies = ((bd.tm_hour*60 + bd.tm_min)*60 + bd.tm_sec)*60 + tv.tv_usec / (1000000/60);

			ram[0xa0] = (unsigned char)(jiffies >> 16);
			ram[0xa1] = (unsigned char)(jiffies >> 8);
			ram[0xa2] = (unsigned char)(jiffies);
	}

	if (addr < ram_size 
		  && (
			  addr < basic_addr // always RAM
			  || (addr >= open_addr && addr < open_addr + open_size) // always open RAM C000.CFFF
			  || (((ram[1] & 3) != 3) && addr >= basic_addr && addr < basic_addr + basic_rom_size) // RAM banked instead of BASIC
			  || (((ram[1] & 2) == 0) && addr >= kernal_addr && addr <= kernal_addr + kernal_rom_size - 1) // RAM banked instead of KERNAL
			  || (((ram[1] & 3) == 0) && addr >= io_addr && addr < io_addr + io_size) // RAM banked instead of IO
		  )
		)
		return ram[addr];
	else if (addr >= basic_addr && addr < basic_addr + basic_rom_size)
		return basic_rom[addr - basic_addr];
	else if (addr >= io_addr && addr < io_addr + io_size)
	{
		if ((ram[1] & 4) == 0)
			return char_rom[addr - io_addr];
		else if (addr >= color_addr && addr < color_addr + color_nybles_size)
			return color_nybles[addr - color_addr] | 0xF0;
		else
			return 0; // io[addr - io_addr];
	}
	else if (addr >= kernal_addr && addr <= kernal_addr + kernal_rom_size - 1)
		return kernal_rom[addr - kernal_addr];
	else
		return 0xFF;
}

void C64Memory::write(ushort addr, byte value)
{
	if (addr <= ram_size-1
		&& (
			addr < io_addr // RAM, including open RAM, and RAM under BASIC
			|| (addr >= kernal_addr && addr <= kernal_addr + kernal_rom_size - 1) // RAM under KERNAL
			|| (((ram[1] & 7) == 0) && addr >= io_addr && addr < io_addr + io_size) // RAM banked in instead of IO
			)
		)
		ram[addr] = value;
	else if (addr == 0xD021) // background
		;
	else if (addr >= color_addr && addr < color_addr + color_nybles_size)
		color_nybles[addr - color_addr] = value;
	//else if (addr >= io_addr && addr < io_addr + io.Length)
	//    io[addr - io_addr] = value;
}
