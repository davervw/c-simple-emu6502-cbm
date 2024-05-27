// emuc128.h - Class EmuC128 - Commodore 128 Emulator
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C Portable Version);
// C64/6502 Emulator for M5Stack Cores
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

#pragma once

#include "emucbm.h"
#include "vicii.h"
#include "vdc.h"

class C128Memory;

class EmuC128 : public EmuCBM
{
public:
	EmuC128();
	virtual ~EmuC128();

protected:
	bool ExecutePatch();

private:
	C128Memory* c128memory; 
	byte GetMemory(ushort addr);
	void SetMemory(ushort addr, byte value);
	void CheckBypassSETNAM();
	void CheckBypassSETLFS();

private:
	EmuC128(const EmuC128& other); // disabled
	bool operator==(const EmuC128& other) const; // disabled
};

class C128Memory : public Emu6502::Memory
{
public:
	C128Memory();
	virtual ~C128Memory();
	virtual byte read(ushort addr);
	virtual void write(ushort addr, byte value);
	bool IsChargen(ushort addr);
	bool IsKernal(ushort addr);
	bool IsBasicHigh(ushort addr);
	bool IsBasicLow(ushort addr);
	bool IsColor(ushort addr);
	bool IsIO(ushort addr);
	bool IsRam(int& addr, bool isWrite);

public:
	byte* basic_lo_rom;
	byte* basic_hi_rom;
	byte* char_rom;
	byte* kernal_rom;
	EmuVicII* vicii;
	VDC8563* vdc;

	static const int basic_lo_size = 0x4000;
	static const int basic_hi_size = 0x4000;
	static const int chargen_size = 0x1000;
	static const int kernal_size = 0x4000;

private:
	byte* ram;
	byte* io;

private:
  void ReadKeyboard();

private:
	C128Memory(const C128Memory& other); // disabled
	bool operator==(const C128Memory& other) const; // disabled
};
