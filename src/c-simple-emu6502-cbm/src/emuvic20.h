#pragma once

#include "emucbm.h"
#include "vic.h"

class EmuVic20 : public EmuCBM
{
public:
	class Vic20Memory : public Memory
	{
	public:
		Vic20Memory(int size);
		virtual ~Vic20Memory();
		byte read(ushort addr);
		void write(ushort addr, byte value);

		byte* ram;
		// ram_lo;         // 1K: 0000-03FF
		// ram_3k;         // 3K: 0400-0FFF (bank0, Optional)
		// ram_default;    // 4K: 1000-1FFF (Always present, default video is 1E00-1FFF)
		// ram_8k1;        // 8K: 2000-3FFF (bank1, Optional)
		// ram_8k2;        // 8K: 4000-5FFF (bank2, Optional)
		// ram_8k3;        // 8K: 6000-7FFF (bank3, Optional)
		byte* char_rom;   // 4K: 8000-8FFF
		byte* io;         // 4K: 9000-9FFF (including video ram)
		// ram_rom         // 8K: A000-BFFF (bank4, Optional, Cartridge)
		byte* basic_rom;  // 8K: C000-DFFF
		byte* kernal_rom; // 8K: E000-FFFF
		byte ram_banks;
		int ram_size;
    EmuVic* vic;
	};

	EmuVic20(int ram_size);
	virtual ~EmuVic20();
	virtual bool ExecutePatch();
};