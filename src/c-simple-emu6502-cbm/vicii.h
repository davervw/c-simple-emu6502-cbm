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

  byte* ram;
  byte* io;
  byte* color_nybles;
  byte* chargen;
  byte* old_video;
  byte* old_color;
  bool postponeDrawChar;
  bool active;
  byte border;
};
