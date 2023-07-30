#pragma once

#include "emu6502.h"

class EmuTedVideo
{
public:
  EmuTedVideo(byte* ram, byte* io, byte* chargen);
  ~EmuTedVideo();
  int TedColorToLCDColor(byte value);
  void DrawChar(byte c, int col, int row, int fg, int bg);
  void DrawChar(int offset);
  void RedrawScreen();
  void RedrawScreenEfficientlyAfterPostponed();
  void DrawBorder(byte value);
  void SaveOldVideoAndColor();

  byte* ram;
  byte* io;
  byte* chargen;
  byte* old_video;
  byte* old_color;
  bool postponeDrawChar;
};
