#pragma once

#include "emu6502.h"

class EmuCBM : public Emu6502
{
public:
	EmuCBM(Memory* mem);
	virtual ~EmuCBM();
	bool LoadPRG(const char* filename);

public:
	static const char* StartupPRG;
	static void File_ReadAllBytes(byte* bytes, unsigned int size, const char* filename);

protected:
	bool ExecutePatch();
	bool ExecuteRTS();
	bool ExecuteJSR(ushort addr);
	bool FileLoad(byte* p_err);
	bool FileSave(const char* filename, ushort addr1, ushort addr2);
	bool LoadStartupPrg();

	int LOAD_TRAP;

	const char* FileName;
	byte FileNum;
	byte FileDev;
	byte FileSec;
	bool FileVerify;
	ushort FileAddr;

private: // disabled
	EmuCBM(const EmuCBM& other); // disabled
	bool operator==(const EmuCBM& other) const; // disabled
};
