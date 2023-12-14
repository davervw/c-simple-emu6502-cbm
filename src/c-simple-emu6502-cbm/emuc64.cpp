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

#include "emuc64.h"
#include "config.h"
#include "cardkbdscan.h"
#ifndef ARDUINO_TEENSY41
#include "ble_keyboard.h"
#endif

// externs (globals)
extern char* StartupPRG;
extern int main_go_num;

// locals
static int startup_state = 0;
static int DRAW_TRAP = -1;

// array allows multiple keys/modifiers pressed at one time
static int scan_codes[16] = { 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64 } ;

EmuC64::EmuC64() : EmuCBM(new C64Memory())
{
  c64memory = (C64Memory*)memory;

  go_state = 0;
}

EmuC64::~EmuC64()
{
}

static void ReadKeyboard()
{
  static const byte extras[24] = {
    64, 27, 16, 64, 59, 11, 24, 56,
    64, 40, 43, 64, 1, 19, 32, 8,
    64, 35, 44, 7, 7, 2, 2, 64
  };

#ifdef ARDUINO_M5STACK_FIRE
  const String upString = "15,7,64";
  const String dnString = "7,64";
  const String crString = "1,64";
  const String runString = "15,63,88";
  const String noString = "64";
  static int lastUp = 1;
  static int lastCr = 1;
  static int lastDn = 1;
  static int lastRun = 1;
#endif

  String s;
#ifndef ARDUINO_TEENSY41
  ble_keyboard->ServiceConnection();
  s = ble_keyboard->Read();
  if (s.length() != 0)
    ;
  else 
#endif
  if (CardKbd)
    s = CardKbdScanRead();
  else if (Serial2.available())
    s = Serial2.readString();
  else if (SerialDef.available())
    s = SerialDef.readString();
#ifdef ARDUINO_M5STACK_FIRE
  else if (lastRun==0 && (lastRun=(digitalRead(39) & digitalRead(38)))==1)
    s = noString;
  else if (lastUp==0 && (lastUp=digitalRead(39))==1)
    s = noString;
  else if (lastCr==0 && (lastCr=digitalRead(38))==1)
    s = noString;
  else if (lastDn==0 && (lastDn=digitalRead(37))==1)
    s = noString;
  else if ((lastRun=(~(~digitalRead(39) & ~digitalRead(38))) & 1)==0)
    s = runString;
  else if ((lastUp=digitalRead(39))==0)
    s = upString;
  else if ((lastCr=digitalRead(38))==0)
    s = crString;
  else if ((lastDn=digitalRead(37))==0)
    s = dnString;
#endif    
  if (s.length() == 0)
    return;

  int src = 0;
  int dest = 0;
  int scan = 0;
  int len = 0;
  while (src < s.length() && dest < 16) {
    char c = s.charAt(src++);
    if (c >= '0' && c <= '9') {
      scan = scan * 10 + (c - '0');
      ++len;
    } else if (len > 0)
    {
      int lobits = scan & 127;
      if (lobits >= 64 && lobits < 88)
      {
        scan = extras[lobits - 64];
        for (int i=0; i<dest; ++i)
          if (scan_codes[i] == 15 || scan_codes[i] == 52)
            scan_codes[i] = 64;
        if (lobits == 83 || lobits == 85)
            scan_codes[dest++] = 15;
      }
      if (scan > 64)
        scan = (scan & 0xFF80) | 0x40;
      scan_codes[dest++] = scan;
      scan = 0;
      len = 0;
    }
  }
  while (dest < 16)
    scan_codes[dest++] = 64;
}

bool EmuC64::ExecutePatch()
{
  static bool NMI = false;
  
  int found_NMI = 0;
  for (int i=0; !found_NMI && i<16; ++i)
    if (scan_codes[i] & 1024)
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
					StartupPRG = 0;
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
  else if (DRAW_TRAP == -1 && !c64memory->vicii->postponeDrawChar &&
    (PC == 0xE8EA // SCROLL SCREEN
    || PC == 0xE965 // INSERT BLANK LINE
    || PC == 0xE9C8 // MOVE SCREEN LINE
    || PC == 0xE9FF // CLEAR SCREEN LINE
    || PC == 0xEA1C)) // STORE A TO SCREEN X TO COLOR (SEE $D1, $F3)
  {
    byte lo = GetMemory((ushort)(0x100 + (S+1)));
    byte hi = GetMemory((ushort)(0x100 + (S+2)));
    DRAW_TRAP = (ushort)(((hi << 8) | lo) + 1); // return address
    c64memory->vicii->postponeDrawChar = true;
    c64memory->vicii->SaveOldVideoAndColor();
  }
  else if (PC == DRAW_TRAP && c64memory->vicii->postponeDrawChar) // returned from drawing postponement
  {
    DRAW_TRAP = -1;
    c64memory->vicii->postponeDrawChar = false;
    c64memory->vicii->RedrawScreenEfficientlyAfterPostponed();
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
	else if (PC == 0xA815) // Execute after GO
	{
		if (go_state == 0 && A >= (byte)'0' && A <= (byte)'9')
		{
			go_state = 1;
			return ExecuteJSR(0xAD8A); // Evaluate expression, check data type
		}
		else if (go_state == 1)
		{
			go_state = 2;
			return ExecuteJSR(0xB7F7); // Convert fp to 2 byte integer
		}
		else if (go_state == 2)
		{
			main_go_num = (ushort)(Y + (A << 8));
			quit = true;
			return true;
		}
	}	

#ifdef ARDUINO_M5STACK_FIRE
  static ushort counter = 0;
  if (counter++ == 0) // infrequently check
  {
    if (digitalRead(37) == 0 && digitalRead(39) == 0) 
    {
      while (digitalRead(37) == 0 || digitalRead(39) == 0); // wait until depress
      main_go_num = 128;
      quit = true;
      return true;
    }
  }
#endif

	return EmuCBM::ExecutePatch();
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

EmuC64::C64Memory::C64Memory()
{
  ram = new byte[ram_size];
  color_nybles = new byte[color_nybles_size];
  io = new byte[io_size];
  basic_rom = new byte[basic_size];
  kernal_rom = new byte[kernal_size];
  chargen_rom = new byte[chargen_size];

  File_ReadAllBytes(basic_rom, basic_size, "/roms/c64/basic");
  File_ReadAllBytes(chargen_rom, chargen_size, "/roms/c64/chargen");
  File_ReadAllBytes(kernal_rom, kernal_size, "/roms/c64/kernal");

  // hack rom to load filename "*" instead of empty filename when SHIFT+RUN
  kernal_rom[0xce8] = 207; // shift O
  kernal_rom[0xce9] = 34;  // double quote
  kernal_rom[0xcea] = 42;  // asterisk

	for (unsigned i = 0; i < ram_size; ++i)
		ram[i] = 0;
	for (unsigned i = 0; i < color_nybles_size; ++i)
		color_nybles[i] = 0;
  for (unsigned i = 0; i < io_size; ++i)
    io[i] = 0;
  
  // initialize DDR and memory mapping to defaults
  ram[0] = 0xEF;
  ram[1] = 0x07;

  vicii = new EmuVicII(ram, io, color_nybles, chargen_rom);
}

EmuC64::C64Memory::~C64Memory()
{
  delete[] ram;
  delete[] color_nybles;
  delete[] io;
  delete[] basic_rom;
  delete[] kernal_rom;
  delete[] chargen_rom;
  delete vicii;
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
        vicii->DrawChar(addr-1024);
    }
  }
  else if (addr == 0xD018) // VIC-II Chip Memory Control Register
  {
    io[addr - io_addr] = value;
    vicii->RedrawScreen(); // upper to lower or lower to upper
  }
  else if (addr == 0xD020) // border
  {
    vicii->DrawBorder(value);
    io[addr - io_addr] = value & 0xF;
  } 
  else if (addr == 0xD021) // background
  {
    io[addr - io_addr] = value & 0xF;
    vicii->RedrawScreen();
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
        vicii->DrawChar(offset);
    }
  }
  else if (addr == 0xDC00)
  {
    io[addr - io_addr] = value;
  } 
}
