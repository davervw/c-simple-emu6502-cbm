// emuc64.h - Commodore 64 Emulator
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C Portable Version)
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

#pragma once

#include "emucbm.h"

class EmuC64 : public EmuCBM
{
public:
	EmuC64(
		int ram_size,
		const char* basic_file,
		const char* chargen_file,
		const char* kernal_file
	);
	~EmuC64();

protected:
	bool ExecutePatch();

private:
	const char* FileName;
	byte FileNum;
	byte FileDev;
	byte FileSec;
	bool FileVerify;
	ushort FileAddr;

private:
	byte GetMemory(ushort addr);
	void SetMemory(ushort addr, byte value);
	bool ExecuteRTS();
	bool ExecuteJSR(ushort addr);
	bool FileLoad(byte* p_err);
	bool FileSave(const char* filename, ushort addr1, ushort addr2);
	bool LoadStartupPrg();
	void CheckBypassSETNAM();
	void CheckBypassSETLFS();

private:
	EmuC64(const EmuC64& other); // disabled
	bool operator==(const EmuC64& other) const; // disabled
};

class C64Memory : public Emu6502::Memory
{
public:
	C64Memory(
		int ram_size,
		const char* basic_file,
		const char* chargen_file,
		const char* kernal_file
	);
	~C64Memory();
	byte read(ushort addr);
	void write(ushort addr, byte value);

private:
	int ram_size;
	byte* ram;
	byte* basic_rom;
	byte* char_rom;
	byte* kernal_rom;
	//byte* io;
	byte* color_nybles;

private:
	C64Memory(const C64Memory& other); // disabled
	bool operator==(const C64Memory& other) const; // disabled
};
