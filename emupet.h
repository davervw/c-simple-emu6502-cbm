#pragma once

#include "emucbm.h"

class EmuPET : public EmuCBM
{
public:
	class PETMemory : public Memory
	{
	private:
		int ram_size;
		byte* ram;
		byte* video_ram;
		byte* io;
		byte* basic;
		byte* edit;
		byte* kernal;
		byte* chargen;
	public:
		PETMemory(int ram_size);
		virtual ~PETMemory();
		virtual byte read(ushort addr);
		virtual void write(ushort addr, byte value);
	};

	EmuPET(int ram_size);
	~EmuPET();
	virtual bool ExecutePatch();
};