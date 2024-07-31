// emumin.h - Minimal 6502 platform
//
// variable RAM, variable ROM, memory mapped Motorola 8250 compatible UART
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C++ Portable Version)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "emu6502.h"
#include "mc6850.h"
#include "terminal.h"

class EmuMinimum : public Emu6502
{
public:
	EmuMinimum(ushort serialaddr);
	virtual ~EmuMinimum();
	bool SaveState(byte*& state, size_t& size);
	bool RestoreState(byte* state, size_t size);

protected:
	bool ExecutePatch();

private:
	byte GetMemory(ushort addr);
	void SetMemory(ushort addr, byte value);
	void ProcessSpecialKey(byte key);
	void LoadOrSaveState(byte key);
	void ResetStateFromDisk(byte*& state, size_t& state_size, int state_num);
	void SaveStateToDisk(byte* state, size_t state_size, int state_num);

private:
	EmuMinimum(const EmuMinimum& other); // disabled
	bool operator==(const EmuMinimum& other) const; // disabled
};

class MinimumMemory : public Emu6502::Memory
{
public:
	MinimumMemory(ushort serialaddr);
	virtual ~MinimumMemory();
	virtual byte read(ushort addr);
	virtual void write(ushort addr, byte value);
	bool SaveState(byte*& state, size_t& size) const;
	bool RestoreState(byte* state, size_t size);
	unsigned getramsize() const;
	unsigned getromsize() const;
	MC6850* uart;
	Terminal* terminal;

private:
	byte* ram;
	ushort ramsize;
	ushort romsize;
	ushort romaddr;
	ushort serialaddr;

private:
	MinimumMemory(const MinimumMemory& other); // disabled
	bool operator==(const MinimumMemory& other) const; // disabled

public:
	inline void CheckPaintFrame(unsigned long timer_now)
	{
#ifdef _WINDOWS
		static unsigned counter = 0;
		if ((++counter & 0x0400) == 0)
			terminal->CheckPaintFrame(timer_now);
#endif // _WINDOWS
	}
};
