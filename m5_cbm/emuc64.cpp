// emuc64.c - Commodore 64 Emulator
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C Portable Version)
// C64/6502 Emulator for M5Stack Cores
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
// MEMORY MAP with implementation notes
//   $00         (data direction missing)
//   $01         Banking implemented (tape sense/controls missing)
//   $0000-$9FFF RAM
//   $A000-$BFFF BASIC ROM
//   $A000-$BFFF Banked LORAM
//   $C000-$CFFF RAM
//   $D000-$D7FF (I/O missing, reads as zeros)
//   $D800-$DFFF VIC-II color RAM nybbles in I/O space (1K x 4bits)
//   $D000-$DFFF Banked RAM
//   $D000-$DFFF Banked Character ROM
//   $E000-$FFFF KERNAL ROM
//   $E000-$FFFF Banked HIRAM
//
// Note it is possible to limit RAM size less than 64K and Commodore ROMs adapt
//
////////////////////////////////////////////////////////////////////////////////
// ROMs copyright Commodore or their assignees
////////////////////////////////////////////////////////////////////////////////

#include "M5Core.h"
#include <FS.h>
#include <SD.h>
#include <SPI.h>

#include "emuc64.h"
#include "emud64.h"

// externs (globals)
extern char* StartupPRG;

// locals
static int startup_state = 0;
static const char* FileName = NULL;
static byte FileNum = 0;
static byte FileDev = 0;
static byte FileSec = 0;
static bool FileVerify = false;
static ushort FileAddr = 0;
static int LOAD_TRAP = -1;
static int DRAW_TRAP = -1;
static EmuD64* disks[4] = {NULL,NULL,NULL,NULL};
static bool postponeDrawChar = false;

// array allows multiple keys/modifiers pressed at one time
static int scan_codes[16] = { 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64 } ;

EmuC64::EmuC64() : Emu6502(new C64Memory())
{
  c64memory = (C64Memory*)memory;

  disks[0] = new EmuD64("/disks/drive8.d64");
  disks[1] = new EmuD64("/disks/drive9.d64");

  // initialize LCD screen
  M5.Lcd.fillScreen(0x0000); // BLACK
}

EmuC64::~EmuC64()
{
  delete memory;
}

static void ReadKeyboard()
{
  if (M5Serial.available()) {
    String s = M5Serial.readString();
    int src = 0;
    int dest = 0;
    int scan = 0;
    while (src < s.length() && dest < 16) {
      char c = s.charAt(src++);
      if (c >= '0' && c <= '9')
        scan = scan * 10 + (c - '0');
      else
      {
        scan_codes[dest++] = scan;
        scan = 0;
      }
    }
    while (dest < 16)
      scan_codes[dest++] = 64;
  }
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

static EmuD64* GetDisk()
{
  if (FileDev == 0 || FileDev == 1)
    return disks[0];
  else if (FileDev >=8 && FileDev <= 11)
    return disks[FileDev-8];
  else
    return 0;
}

static byte* OpenRead(const char* filename, int* p_ret_file_len)
{
  static const int read_buffer_size = 65536;
	static unsigned char* read_buffer = new unsigned char[read_buffer_size];

  EmuD64* disk = GetDisk();
  if (disk == 0)
  {
      *p_ret_file_len = 0;
      return (byte*)NULL;
  }

	if (filename != NULL && filename[0] == '$' && filename[1] == '\0')
	{
		*p_ret_file_len = read_buffer_size;
		if (disk->GetDirectoryProgram(read_buffer, *p_ret_file_len))
			return &read_buffer[0];
		else
		{
      *p_ret_file_len = 0;
			return (byte*)NULL;
		}
	}
	else
	{
		*p_ret_file_len = read_buffer_size;
		disk->ReadFileByName(filename, read_buffer, *p_ret_file_len);
    if (*p_ret_file_len == 0)
       return (byte*)NULL;
    else
		   return &read_buffer[0];
	}
}

// returns success
bool EmuC64::FileLoad(byte* p_err)
{
	bool startup = (StartupPRG != NULL);
	ushort addr = FileAddr;
	bool success = true;
	const char* filename = (StartupPRG != NULL) ? StartupPRG : FileName;
	int file_len;
	byte* bytes = OpenRead(filename, &file_len);
	ushort si = 0;
	if (file_len == 0) {
		*p_err = 4; // FILE NOT FOUND
  	FileAddr = addr;
		return false;
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
  EmuD64* disk = GetDisk();
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
	disk->StoreFileByName(filename, bytes, len);
	free(bytes);
	return true;
}

bool EmuC64::LoadStartupPrg()
{
	bool result;
	byte err;
  EmuD64* disk = GetDisk();
  if (disk == 0)
    return false;
	result = FileLoad(&err);
	if (!result)
		return false;
	else
		return FileSec == 0 ? true : false; // relative is BASIC, absolute is ML
}

// forward declaration
static void CheckBypassSETNAM();
static void CheckBypassSETLFS();

bool EmuC64::ExecutePatch()
{
  static bool NMI = false;
  
  int found_NMI = 0;
  for (int i=0; !found_NMI && i<16; ++i)
    if (scan_codes[i] == 1024+64)
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

					return true; // overriden, and PC changed, so caller should reloop before execution to allow breakpoint/trace/ExecutePatch/etc.
				}
			}
			else {
				FileName = StartupPRG;
				FileAddr = GetMemory(43) | (GetMemory(44) << 8);
				is_basic = LoadStartupPrg();
			}

			StartupPRG = 0;

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
      SetA(8); // MISSING file name message
      C = true; // failure
    }
		else if (A == 0 || A == 1) {
			LOAD_TRAP = PC;
			C = false; // success
		}
		else {
      FileName = "";
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
  else if (DRAW_TRAP == -1 && !postponeDrawChar &&
    (PC == 0xE8EA // SCROLL SCREEN
    || PC == 0xE965 // INSERT BLANK LINE
    || PC == 0xE9C8 // MOVE SCREEN LINE
    || PC == 0xE9FF // CLEAR SCREEN LINE
    || PC == 0xEA1C)) // STORE A TO SCREEN X TO COLOR (SEE $D1, $F3)
  {
    byte lo = GetMemory((ushort)(0x100 + (S+1)));
    byte hi = GetMemory((ushort)(0x100 + (S+2)));
    DRAW_TRAP = (ushort)(((hi << 8) | lo) + 1); // return address
    postponeDrawChar = true;
    c64memory->SaveOldVideoAndColor();
  }
  else if (PC == DRAW_TRAP && postponeDrawChar) // returned from drawing postponement
  {
    DRAW_TRAP = -1;
    postponeDrawChar = false;
    c64memory->RedrawScreenEfficientlyAfterPostponed();
  }
	else if (GetMemory(PC) == 0x6C && GetMemory((ushort)(PC + 1)) == 0x30 && GetMemory((ushort)(PC + 2)) == 0x03) // catch JMP(LOAD_VECTOR), redirect to jump table
	{
		CheckBypassSETLFS();
		CheckBypassSETNAM();
		// note: A register has same purpose LOAD/VERIFY
		X = GetMemory(0xC3);
		Y = GetMemory(0xC4);
		PC = 0xFFD5; // use KERNAL JUMP TABLE instead, so LOAD is hooked by base
		return true; // re-execute
	}
	else if (GetMemory(PC) == 0x6C && GetMemory((ushort)(PC + 1)) == 0x32 && GetMemory((ushort)(PC + 2)) == 0x03) // catch JMP(SAVE_VECTOR), redirect to jump table
	{
		CheckBypassSETLFS();
		CheckBypassSETNAM();
		X = GetMemory(0xAE);
		Y = GetMemory(0xAF);
		A = 0xC1;
		PC = 0xFFD8; // use KERNAL JUMP TABLE instead, so SAVE is hooked by base
		return true; // re-execute
	}

	return false; // execute normally
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

// note ram starts at 0x0000
static const int basic_addr = 0xA000;
static const int basic_size = 0x2000;
static const int kernal_addr = 0xE000;
static const int kernal_size = 0x2000;
static const int io_addr = 0xD000;
static const int io_size = 0x1000;
static const int chargen_size = 0x1000;
static const int color_addr = 0xD800;
static const int open_addr = 0xC000;
static const int open_size = 0x1000;
static const int ram_size = 64 * 1024;
const int color_nybles_size = 1024;

static void File_ReadAllBytes(byte* bytes, int size, const char* filename)
{
    File fp = SD.open(filename, FILE_READ);
    if (!fp)
      return;
    fp.read(bytes, size);
    fp.close();
}

EmuC64::C64Memory::C64Memory()
{
  ram = new byte[ram_size];
  color_nybles = new byte[color_nybles_size];
  io = new byte[io_size];
  basic_rom = new byte[basic_size];
  kernal_rom = new byte[kernal_size];
  chargen_rom = new byte[chargen_size];
  old_video = new byte[1000];
  old_color = new byte[1000];

  File_ReadAllBytes(basic_rom, basic_size, "/roms/c64/basic");
  File_ReadAllBytes(chargen_rom, chargen_size, "/roms/c64/chargen");
  File_ReadAllBytes(kernal_rom, kernal_size, "/roms/c64/kernal");

	for (unsigned i = 0; i < ram_size; ++i)
		ram[i] = 0;
	for (unsigned i = 0; i < color_nybles_size; ++i)
		color_nybles[i] = 0;
  for (unsigned i = 0; i < io_size; ++i)
    io[i] = 0;
  
  // initialize DDR and memory mapping to defaults
  ram[0] = 0xEF;
  ram[1] = 0x07;
}

EmuC64::C64Memory::~C64Memory()
{
  delete[] ram;
  delete[] color_nybles;
  delete[] io;
  delete[] basic_rom;
  delete[] kernal_rom;
  delete[] chargen_rom;
  delete[] old_video;
  delete[] old_color;
}

// RGB565 colors picked with http://www.barth-dev.de/online/rgb565-color-picker/
int EmuC64::C64ColorToLCDColor(byte value)
{
  switch (value & 0xF)
  {
    case 0: return TFT_BLACK;
    case 1: return TFT_WHITE;
    case 2: return TFT_RED;
    case 3: return TFT_CYAN;
    case 4: return 0x8118; // DARKMAGENTA OR PURPLE
    case 5: return 0x0400; // DARKGREEN
    case 6: return TFT_BLUE;
    case 7: return TFT_YELLOW;
    case 8: return TFT_ORANGE;
    case 9: return 0x8283; // BROWN;
    case 10: return 0xFC10; // PINK OR LT RED
    case 11: return TFT_DARKGREY;
    case 12: return TFT_DARKCYAN; // MED GRAY
    case 13: return 0x07E0; // LIGHTGREEN;
    case 14: return 0x841F; // LIGHTBLUE;
    case 15: return TFT_LIGHTGREY;
    default: return 0;
  }
}

void EmuC64::C64Memory::DrawChar(byte c, int col, int row, int fg, int bg)
{
  if (postponeDrawChar)
    return;
  
  int offset = ((io[0x18] & 2) == 0) ? 0 : (8*256);
  const byte* shape = &chargen_rom[c*8+offset];
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

void EmuC64::C64Memory::DrawChar(int offset)
{
  int col = offset % 40;
  int row = offset / 40;
  int fg = EmuC64::C64ColorToLCDColor(color_nybles[offset]);
  int bg = EmuC64::C64ColorToLCDColor(io[0x21]);
  DrawChar(ram[1024+offset], col, row, fg, bg);
}

void EmuC64::C64Memory::RedrawScreen()
{
  int bg = EmuC64::C64ColorToLCDColor(io[0x21]);
  int offset = 0;
  for (int row = 0; row < 25; ++row)
  {
    for (int col = 0; col < 40; ++col)
    {
      int fg = EmuC64::C64ColorToLCDColor(color_nybles[offset]);
      DrawChar(ram[1024 + offset], col, row, fg, bg);
      ++offset;
    }
  }
}

void EmuC64::C64Memory::RedrawScreenEfficientlyAfterPostponed()
{
  int bg = EmuC64::C64ColorToLCDColor(io[0x21]);
  int offset = 0;
  for (int row = 0; row < 25; ++row)
  {
    for (int col = 0; col < 40; ++col)
    {
      int fg = EmuC64::C64ColorToLCDColor(color_nybles[offset]);
      byte old_char = old_video[offset];
      byte new_char = ram[1024 + offset];
      byte old_fg_index = old_color[offset] & 0xF;
      byte new_fg_index = color_nybles[offset] & 0xF;
      if (old_char != new_char || old_fg_index != new_fg_index)
        DrawChar(new_char, col, row, fg, bg);
      ++offset;
    }
  }
}

byte EmuC64::C64Memory::read(ushort addr)
{
  if (addr <= ram_size - 1
      && (
        addr < basic_addr // always RAM
        || (addr >= open_addr && addr < open_addr + open_size) // always open RAM C000.CFFF
        || (((ram[1] & 3) != 3) && addr >= basic_addr && addr < basic_addr + basic_size) // RAM banked instead of BASIC
        || (((ram[1] & 2) == 0) && addr >= kernal_addr && addr <= kernal_addr + kernal_size - 1) // RAM banked instead of KERNAL
        || (((ram[1] & 3) == 0) && addr >= io_addr && addr < io_addr + io_size) // RAM banked instead of IO
      )
    )
    return ram[addr];
  else if (addr >= basic_addr && addr < basic_addr + basic_size)
    return basic_rom[addr - basic_addr];
  else if (addr >= io_addr && addr < io_addr + io_size)
  {
    if ((ram[1] & 4) == 0)
      return chargen_rom[addr - io_addr];
    else if (addr >= color_addr && addr < color_addr + color_nybles_size)
      return color_nybles[addr - color_addr] | 0xF0;
    else if (addr == 0xDC01)
    {
      ReadKeyboard();

      int value = 0;
      
      for (int i=0; i<16; ++i)
      {
        int scan_code = scan_codes[i] & 127; // remove any modifiers
        if (scan_code < 64)
        {     
          int col = scan_code / 8;
          int row = scan_code % 8;
          
          if ((io[0xC00] & (1 << col)) == 0)
            value |= (1 << row);
        }
      }
      
      return ~value;
    }
    else
      return io[addr - io_addr];
  }
  else if (addr >= kernal_addr && addr <= kernal_addr + kernal_size-1)
    return kernal_rom[addr - kernal_addr];
  else
    return 0xFF;
}

void EmuC64::C64Memory::write(ushort addr, byte value)
{
  if (addr <= ram_size - 1
    && (
      addr < io_addr // RAM, including open RAM, and RAM under BASIC
      || (addr >= kernal_addr && addr <= kernal_addr + kernal_size - 1) // RAM under KERNAL
      || (((ram[1] & 7) == 0) && addr >= io_addr && addr < io_addr + io_size) // RAM banked in instead of IO
      )
    )
  {
    bool changedRequiresUpdate = ram[addr] != value;
    if (changedRequiresUpdate)
    {
      ram[addr] = value;
      if (addr >= 1024 && addr < 2024)
        DrawChar(addr-1024);
    }
  }
  else if (addr == 0xD018) // VIC-II Chip Memory Control Register
  {
    io[addr - io_addr] = value;
    RedrawScreen(); // upper to lower or lower to upper
  }
  else if (addr == 0xD020) // border
  {
    int border = EmuC64::C64ColorToLCDColor(value);
    M5.Lcd.fillRect(0, 0, 320, 20, border);
    M5.Lcd.fillRect(0, 220, 320, 20, border);
    io[addr - io_addr] = value & 0xF;
  } 
  else if (addr == 0xD021) // background
  {
    io[addr - io_addr] = value & 0xF;
    RedrawScreen();
  }
  else if (addr >= color_addr && addr < color_addr + color_nybles_size)
  {
    int offset = addr - color_addr;
    bool colorChange = (color_nybles[offset] != value);
    if (colorChange)
    {
      color_nybles[offset] = value;
      char charAtOffset = ram[1024 + offset];
      bool isBlank = (charAtOffset == ' ' || charAtOffset == 96);
      bool requiresRedraw = !isBlank;
      if (requiresRedraw)
        DrawChar(offset);
    }
  }
  else if (addr == 0xDC00)
  {
    io[addr - io_addr] = value;
  } 
}

void EmuC64::C64Memory::SaveOldVideoAndColor()
{
    memcpy(&old_video[0], &ram[1024], 1000);
    memcpy(&old_color[0], &color_nybles[0], 1000);
}
