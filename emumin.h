#pragma once

#include "emu6502.h"
#include "mc6850.h"

class EmuMinimum : public Emu6502
{
public:
	EmuMinimum(const char* filename, ushort serialaddr);
	virtual ~EmuMinimum();

protected:
	bool ExecutePatch();

private:
	byte GetMemory(ushort addr);
	void SetMemory(ushort addr, byte value);

private:
	EmuMinimum(const EmuMinimum& other); // disabled
	bool operator==(const EmuMinimum& other) const; // disabled
};

class MinimumMemory : public Emu6502::Memory
{
public:
	MinimumMemory(const char* filename, ushort serialaddr);
	virtual ~MinimumMemory();
	virtual byte read(ushort addr);
	virtual void write(ushort addr, byte value);
	unsigned getramsize();
	unsigned getromsize();

private:
	byte* ram;
	ushort ramsize;
	ushort romsize;
	ushort romaddr;
	ushort serialaddr;
	MC6850 uart;

private:
	MinimumMemory(const MinimumMemory& other); // disabled
	bool operator==(const MinimumMemory& other) const; // disabled
};
