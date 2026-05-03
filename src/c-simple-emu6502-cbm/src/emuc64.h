// emuc64.h - Commodore 64 Emulator
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C Portable Version);
// C64/6502 Unified Emulator for M5Stack/Teensy/ESP32 LCDs and Windows
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

#include "emucbm.h"
#include "vicii.h"

class EmuC64 : public EmuCBM
{
  class C64Memory : public Memory
  {
  public:
    C64Memory();
    ~C64Memory();
    virtual byte read(ushort addr);
    virtual void write(ushort addr, byte value);
    void DrawChar(byte c, int col, int row, int fg, int bg);
    void DrawChar(int offset);
    void RedrawScreen();
    void RedrawScreenEfficientlyAfterPostponed();
    void SaveOldVideoAndColor();

  private:
    byte* ram;
    byte* color_nybles;
    byte* io;
    byte* basic_rom;
    byte* kernal_rom;
    byte* chargen_rom;

  public:
    EmuVicII* vicii;

  private:
    C64Memory(const C64Memory& other); // disabled
    bool operator==(const C64Memory& other) const; // disabled    
  };

public:
  EmuC64();
  virtual ~EmuC64();

protected:
  C64Memory* c64memory;
  int go_state;

protected:
  bool ExecutePatch();
  void CheckBypassSETNAM();
  void CheckBypassSETLFS();
  static int C64ColorToLCDColor(byte value);
};
