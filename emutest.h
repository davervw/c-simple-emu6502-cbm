#pragma once

#include "emu6502.h"

class EmuTest : public Emu6502
{
public:
	EmuTest(const char* filename);
	virtual ~EmuTest();

protected:
	bool ExecutePatch();

private:
	byte GetMemory(ushort addr);
	void SetMemory(ushort addr, byte value);

private:
	EmuTest(const EmuTest& other); // disabled
	bool operator==(const EmuTest& other) const; // disabled
};

class TestMemory : public Emu6502::Memory
{
public:
	TestMemory(const char* filename);
	virtual ~TestMemory();
	virtual byte read(ushort addr);
	virtual void write(ushort addr, byte value);

private:
	byte* ram;

private:
	TestMemory(const TestMemory& other); // disabled
	bool operator==(const TestMemory& other) const; // disabled
};
