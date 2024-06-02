// vicii.h - C64 VIC-II Video Chip Emulation
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C Portable Version)
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

#include "emu6502.h"

class EmuVicII
{
public:
  EmuVicII(byte* ram, byte* io, byte* color_nybles, byte* chargen);
  ~EmuVicII();
  int C64ColorToLCDColor(byte value);
  void DrawChar(byte c, int col, int row, int fg, int bg);
  void DrawChar(int offset);
  void RedrawScreen();
  void RedrawScreenEfficientlyAfterPostponed();
  void DrawBorder(byte value);
  void SaveOldVideoAndColor();
  void Activate();
  void Deactivate();
  void UpdateAddresses();

#ifdef _WINDOWS
  void CheckPaintFrame(unsigned long micros_now);
  bool needsPaintFrame;
  unsigned long lastPaintFrame;
  static const long paintFrameInterval = 1000000 / 60; // TODO: have LCDs employ this technique for more optimal screen refreshes (screen scrolling, and other high rate updates)
  bool redrawRequiredSignal;
#endif // _WINDOWS

  byte* ram;
  byte* io;
  byte* color_nybles;
  byte* chargen;
  byte* old_video;
  byte* old_color;
  bool postponeDrawChar;
  bool active;
  byte border;
  ushort chargen_addr;
  ushort video_addr;
};
