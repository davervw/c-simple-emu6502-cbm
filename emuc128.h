#pragma once

#include "emucbm.h"

class C128Memory;

class EmuC128 : public EmuCBM
{
public:
	EmuC128(
		const char* basic_lo_file,
		const char* basic_hi_file,
		const char* chargen_file,
		const char* kernal_file
	);
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

class VDC8563
{
private:
	byte* registers;
	byte* vdc_ram;
	byte register_addr;
	byte data;
	bool ready = false;

public:
	VDC8563();
	~VDC8563();

	byte GetAddressRegister();
	void SetAddressRegister(byte value);
	byte GetDataRegister();
	void SetDataRegister(byte value);
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

	static const int basic_lo_size = 0x4000;
	static const int basic_hi_size = 0x4000;
	static const int chargen_size = 0x1000;
	static const int kernal_size = 0x4000;

private:
	byte* ram;
	byte* io;
	byte* color_nybles;
	VDC8563* vdc;

private:
	C128Memory(const C128Memory& other); // disabled
	bool operator==(const C128Memory& other) const; // disabled
};
