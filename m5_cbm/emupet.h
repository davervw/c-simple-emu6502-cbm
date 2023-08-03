#pragma once

#include "emucbm.h"

class EmuPET : public EmuCBM
{
public:
  class PETVideo
  {
  public:
    PETVideo(byte* video_ram);
    ~PETVideo();
    void DrawChar(byte c, int col, int row, int fg, int bg);
    void DrawChar(int offset);
    void RedrawScreen();
    void DrawBorder();
  private:
    byte* video_ram;
    byte* chargen;
  };

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
    PETVideo* video;
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
