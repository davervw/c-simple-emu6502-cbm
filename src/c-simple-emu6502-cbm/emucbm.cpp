// emucbm.cpp - class EmuCBM - Commodore Business Machines Emulator
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C Portable Version);
// C64/6502 Unified Emulator for M5Stack/Teensy/ESP32 LCDs and Windows
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

#include "emucbm.h"
#include "emud64.h"

#ifdef _WINDOWS
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include "WindowsFile.h"
#else // NOT _WINDOWS
#include <FS.h>
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
#include "FFat.h"
#else
#include <SD.h>
#include <SPI.h>
#endif
#endif // NOT _WINDOWS
#include "config.h"

// externs (globals)
extern const char* StartupPRG;

static EmuD64* disks[4] = {NULL,NULL,NULL,NULL};

EmuCBM::EmuCBM(Memory* memory): Emu6502(memory)
{
  if (disks[0] == NULL)
    disks[0] = new EmuD64("/disks/drive8.d64");
  if (disks[1] == NULL)
    disks[1] = new EmuD64("/disks/drive9.d64");

  LOAD_TRAP = -1;
  FileName = NULL;
  FileNum = 0;
  FileDev = 0;
  FileSec = 0;
  FileVerify = false;
  FileAddr = 0;
}

EmuCBM::~EmuCBM()
{
}

EmuD64* EmuCBM::GetDisk()
{
  if (FileDev == 0 || FileDev == 1)
    return disks[0];
  else if (FileDev >=8 && FileDev <= 11)
    return disks[FileDev-8];
  else
    return 0;
}

byte* EmuCBM::OpenRead(const char* filename, int* p_ret_file_len)
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
bool EmuCBM::FileLoad(byte* p_err)
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

bool EmuCBM::FileSave(const char* filename, ushort addr1, ushort addr2)
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

bool EmuCBM::LoadStartupPrg()
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

bool EmuCBM::ExecutePatch()
{
	if (PC == 0xFFBA) // SETLFS
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

  return false;
}

bool EmuCBM::ExecuteRTS()
{
	byte bytes;
	RTS(&PC, &bytes);
	return true; // return value for ExecutePatch so will reloop execution to allow berakpoint/trace/ExecutePatch/etc.
}

bool EmuCBM::ExecuteJSR(ushort addr)
{
	ushort retaddr = (PC - 1) & 0xFFFF;
	Push(HI(retaddr));
	Push(LO(retaddr));
	PC = addr;
	return true; // return value for ExecutePatch so will reloop execution to allow berakpoint/trace/ExecutePatch/etc.
}

int EmuCBM::File_ReadAllBytes(byte* bytes, int size, const char* filename)
{
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    File fp = FFat.open(filename, FILE_READ);
#else
    File fp = SD.open(filename, FILE_READ);
#endif    
	if (!fp)
		return 0;
	int filesize = fp.size();
	fp.read(bytes, size);
    fp.close();
	return filesize;
}
