#pragma once

#include "emu6502.h"

class EmuVic
{
public:
  EmuVic(byte* ram, byte* io, byte* chargen);
  ~EmuVic();
  int Vic20ColorToLCDColor(byte value);
  void DrawChar(byte c, int col, int row, int fg, int bg);
  void DrawChar(int offset);
  void RedrawScreen();
  void RedrawScreenEfficientlyAfterPostponed();
  void DrawBorder(byte value);
  void SaveOldVideoAndColor();

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
  ushort video_ram_addr;
  ushort color_ram_addr;
};
