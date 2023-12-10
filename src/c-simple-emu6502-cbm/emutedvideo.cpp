#include "emutedvideo.h"
#include "config.h"

static const int color_addr = 0x0800;
static const int chars_addr = 0x0C00;

EmuTedVideo::EmuTedVideo(byte* vram, byte* vio, byte* vchargen)
{
  ram = vram;
  io = vio;
  chargen = vchargen;
  old_video = new byte[1000];
  old_color = new byte[1000];
  postponeDrawChar = false;

  memset(old_video, 32, 1000);
  memset(old_color, 0, 1000);

  // initialize LCD screen
  M5.Lcd.fillScreen(0x0000);  // BLACK
}

EmuTedVideo::~EmuTedVideo()
{
  delete [] old_video;
  delete [] old_color;
}

// RGB565 colors picked with http://www.barth-dev.de/online/rgb565-color-picker/
int EmuTedVideo::TedColorToLCDColor(byte value)
{
  switch (value & 0xF)
  {
    case 0: return TFT_BLACK;
    case 1: return TFT_WHITE;
    case 2: return TFT_RED;
    case 3: return TFT_CYAN;
    case 4: return 0x8118; // DARKMAGENTA OR PURPLE
    case 5: return 0x0400; // DARKGREEN
    case 6: return TFT_BLUE;
    case 7: return TFT_YELLOW;
    case 8: return TFT_ORANGE;
    case 9: return 0x8283; // BROWN
    case 10: return 0x3BC0; // YELLOWGREEN
    case 11: return 0xFC10; // PINK OR LT RED
    case 12: return 0x07C7; // BLUEGREEN
    case 13: return 0x841F; // LIGHTBLUE
    case 14: return 0x781F; // PURPLE
    case 15: return 0xBFF7; // LIGHTGREEN
    default: return 0;
  }
}

void EmuTedVideo::DrawChar(byte c, int col, int row, int fg, int bg)
{
  if (postponeDrawChar)
    return;
  
  //int offset = ((io[0x18] & 2) == 0) ? 0 : (8*256);
  int offset = 0;
  const byte* shape = &chargen[c*8+offset];
  int x0 = 0 + col*8;
  int y0 = 20 + row*8;
  for (int row_i=0; row_i<8; ++row_i)
  {
    int mask = 128;
    for (int col_i=0; col_i<8; ++col_i)
    {
      M5.Lcd.drawPixel(x0+col_i, y0+row_i, ((shape[row_i] & mask) == 0) ? bg : fg);
      mask = mask >> 1;
    }
  }
}

void EmuTedVideo::DrawChar(int offset)
{
  int col = offset % 40;
  int row = offset / 40;
  int fg = TedColorToLCDColor(ram[color_addr+offset]);
  int bg = TedColorToLCDColor(io[0x215]);
  DrawChar(ram[chars_addr+offset], col, row, fg, bg);
}

void EmuTedVideo::RedrawScreen()
{
  int bg = TedColorToLCDColor(io[0x215]);
  int offset = 0;
  for (int row = 0; row < 25; ++row)
  {
    for (int col = 0; col < 40; ++col)
    {
      int fg = TedColorToLCDColor(ram[color_addr+offset]);
      DrawChar(ram[chars_addr + offset], col, row, fg, bg);
      ++offset;
    }
  }
}

void EmuTedVideo::RedrawScreenEfficientlyAfterPostponed()
{
  int bg = TedColorToLCDColor(io[0x215]);
  int offset = 0;
  for (int row = 0; row < 25; ++row)
  {
    for (int col = 0; col < 40; ++col)
    {
      int fg = TedColorToLCDColor(ram[color_addr+offset]);
      byte old_char = old_video[offset];
      byte new_char = ram[chars_addr + offset];
      byte old_fg_index = old_color[offset] & 0xF;
      byte new_fg_index = ram[color_addr+offset] & 0xF;
      if (old_char != new_char || old_fg_index != new_fg_index)
        DrawChar(new_char, col, row, fg, bg);
      ++offset;
    }
  }
}

void EmuTedVideo::DrawBorder(byte value)
{
    int color = TedColorToLCDColor(value);
    M5.Lcd.fillRect(0, 0, 320, 20, color);
    M5.Lcd.fillRect(0, 220, 320, 20, color);
}

void EmuTedVideo::SaveOldVideoAndColor()
{
    memcpy(&old_video[0], &ram[chars_addr], 1000);
    memcpy(&old_color[0], &ram[color_addr], 1000);
}