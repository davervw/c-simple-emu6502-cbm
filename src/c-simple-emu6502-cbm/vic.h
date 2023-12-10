#pragma once

#include "emu6502.h"

class EmuVic
{
public:
  EmuVic(byte* ram, byte* io, byte* color_nybles, byte* chargen);
  ~EmuVic();
  int Vic20ColorToLCDColor(byte value);
  void DrawChar(byte c, int col, int row, int fg, int bg);
  void DrawChar(int offset);
  void RedrawScreen();
  void RedrawScreenEfficientlyAfterPostponed();
  void DrawBorder(byte value);
  void SaveOldVideoAndColor();

  byte* ram;
  byte* io;
  byte* color_nybles;
  byte* chargen;
  byte* old_video;
  byte* old_color;
  bool postponeDrawChar;
};
