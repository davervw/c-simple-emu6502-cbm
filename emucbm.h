#pragma once

#include "emu6502.h"

class EmuCBM : public Emu6502
{
public:
	EmuCBM(Memory* mem);
	bool LoadPRG(const char* filename);

protected:
	bool ExecutePatch();

private:
	EmuCBM(const EmuCBM& other); // disabled
	bool operator==(const EmuCBM& other) const; // disabled
};
