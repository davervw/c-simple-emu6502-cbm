#include "Vic.h"
#include "M5Core.h"

//#define VIC1TO1 // 1 to 1 pixels horizontally and vertically
#define VIC3TO2 //3 pixels for every 2 pixels horizontally, 1 to 1 pixels vertically

#ifdef VIC1TO1
const int X0 = 72;
const int Y0 = 28;
#endif

#ifdef VIC3TO2
const int X0 = 28;
const int Y0 = 28;
#endif

EmuVic::EmuVic(byte* vram, byte* vio, byte* vcolor_nybles, byte* vchargen)
{
  ram = vram;
  io = vio;
  color_nybles = vcolor_nybles;
  chargen = vchargen;
  old_video = new byte[22*23];
  old_color = new byte[22*23];
  postponeDrawChar = false;

  memset(old_video, 32, 22*23);
  memset(old_color, 0, 22*23);

  // initialize LCD screen
  M5.Lcd.fillScreen(0x0000);  // BLACK
}

EmuVic::~EmuVic()
{
  delete [] old_video;
  delete [] old_color;
}

 // RGB565 colors picked with http://www.barth-dev.de/online/rgb565-color-picker/
 // Vic-20 pallette assistance from https://www.deviantart.com/thewolfbunny64/art/Commodore-VIC-20-Color-Palettes-860440244
int EmuVic::Vic20ColorToLCDColor(byte value)
{
  switch (value & 0xF)
  {
    case 0: return 0x0000; // BLACK
    case 1: return 0xFFFF; // WHITE
    case 2: return 0x8800; // RED
    case 3: return 0xA77E; // CYAN
    case 4: return 0x8118; // DARKMAGENTA OR PURPLE
    case 5: return 0x3E03; // GREEN
    case 6: return 0x0015; // BLUE
    case 7: return TFT_YELLOW;
    case 8: return TFT_ORANGE; // ORANGE
    case 9: return 0xFFFA; // PALE YELLOW
    case 10: return 0xFC10; // PINK OR LT RED
    case 11: return 0xB73F; // LIGHTBLUE
    case 12: return 0xF61F; // LIGHTMAGENTA
    case 13: return 0xBFF7; // LIGHTGREEN
    case 14: return 0xB73F; // LIGHTBLUE
    case 15: return 0xDF50; // YELLOW BROWN 
    default: return 0;
  }
}

void EmuVic::DrawChar(byte c, int col, int row, int fg, int bg)
{
  if (postponeDrawChar)
    return;

  M5.Lcd.startWrite();
  int offset = ((io[0x5] & 2) == 0) ? 0 : (8*256);
  const byte* shape = &chargen[c*8+offset];
  int y0 = Y0 + row*8;
#ifdef VIC1TO1  
  int x0 = X0 + col*8;
  int maxcol = 8;
#endif
#ifdef VIC3TO2
  int x0 = X0 + col*12;
  int midcolor = bg;
  int maxcol = 12;
  int last_color;
#endif
  for (int row_i=0; row_i<8; ++row_i)
  {
    int mask = 128;
    for (int col_i=0; col_i<maxcol; ++col_i)
    {
      int color = ((shape[row_i] & mask) == 0) ? bg : fg;
  #ifdef VIC1TO1
      mask = mask >> 1;
  #endif
  #ifdef VIC3TO2
      int frac = col_i % 3;
      if (frac == 1)
      {
        if (color != last_color)
        {
          // extract 565 color from 16-bit values, average them, and combine back the same way
          int red = ((color >> 11) + (last_color >> 11)) / 2;
          int green = (((color >> 5) & 0x3F) + (((last_color >> 5) & 0x3F))) / 2;
          int blue = ((color & 0x1F) + (last_color & 0x1F)) / 2;
          color = ((red & 0x1f) << 11) | ((green & 0x3f) << 5) | (blue & 0x1F);
        }
      }
      else
      {
        mask = mask >> 1;
        last_color = color;
      }
  #endif
      M5.Lcd.drawPixel(x0+col_i, y0+row_i, color);
    }
  }
  M5.Lcd.endWrite();
}

void EmuVic::DrawChar(int offset)
{
  int col = offset % 22;
  int row = offset / 22;
  int fg = EmuVic::Vic20ColorToLCDColor(color_nybles[offset] & 7);
  int bg = EmuVic::Vic20ColorToLCDColor(io[0xf] >> 4);
  DrawChar(ram[0x1E00+offset], col, row, fg, bg);
}

void EmuVic::RedrawScreen()
{
  M5.Lcd.startWrite();
  int bg = EmuVic::Vic20ColorToLCDColor(io[0xf] >> 4);
  int offset = 0;
  for (int row = 0; row < 23; ++row)
  {
    for (int col = 0; col < 22; ++col)
    {
      int fg = EmuVic::Vic20ColorToLCDColor(color_nybles[offset] & 7);
      DrawChar(ram[0x1E00 + offset], col, row, fg, bg);
      ++offset;
    }
  }
  M5.Lcd.endWrite();
}

void EmuVic::RedrawScreenEfficientlyAfterPostponed()
{
  M5.Lcd.startWrite();
  int bg = EmuVic::Vic20ColorToLCDColor(io[0x0f] >> 4);
  int offset = 0;
  for (int row = 0; row < 23; ++row)
  {
    for (int col = 0; col < 22; ++col)
    {
      int fg = EmuVic::Vic20ColorToLCDColor(color_nybles[offset]);
      byte old_char = old_video[offset];
      byte new_char = ram[0x1E00 + offset];
      byte old_fg_index = old_color[offset] & 0xF;
      byte new_fg_index = color_nybles[offset] & 0xF;
      if (old_char != new_char || old_fg_index != new_fg_index)
        DrawChar(new_char, col, row, fg, bg);
      ++offset;
    }
  }
  M5.Lcd.endWrite();
}

void EmuVic::DrawBorder(byte value)
{
    M5.Lcd.startWrite();
    int color = Vic20ColorToLCDColor(value & 7);
    M5.Lcd.fillRect(0, 0, 320, Y0, color);
    M5.Lcd.fillRect(0, Y0, X0, 23*8, color);
    M5.Lcd.fillRect(320-X0, Y0, X0, 23*8, color);
    M5.Lcd.fillRect(0, 240-Y0, 320, Y0, color);
    M5.Lcd.endWrite();
}

void EmuVic::SaveOldVideoAndColor()
{
    memcpy(&old_video[0], &ram[0x1E00], 22*23);
    memcpy(&old_color[0], &color_nybles[0], 22*23);
}