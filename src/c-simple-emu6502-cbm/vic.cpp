#include "vic.h"
#include "config.h"

#ifdef M5STACK
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
#endif

#ifdef ARDUINO_SUNTON_8048S070
#define VIC1TO1 // 1 to 1 pixels horizontally and vertically
const int X0 = 48;
const int Y0 = 56;
#endif

#ifdef ARDUINO_TEENSY41
 #ifdef ILI9341
#define VIC3TO2
const int X0 = 28;
const int Y0 = 28;
 #endif
 #ifdef ILI9488
#define VIC1TO1
const int X0 = 20;
const int Y0 = 22;
 #endif
#endif

#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
const int X0 = 6;
const int Y0 = 5;
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
#ifdef M5STACK  
  M5.Lcd.fillScreen(0x0000);  // BLACK
#endif
#ifdef ARDUINO_SUNTON_8048S070
  gfx->fillScreen(0x0000);  // BLACK
#endif  
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
#ifdef M5STACK
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
#endif
#ifdef ARDUINO_SUNTON_8048S070
    case 0: return 0x0000; // BLACK
    case 1: return 0xFFFF; // WHITE
    case 2: return 0x8800; // RED
    case 3: return 0xA77E; // CYAN
    case 4: return 0x8118; // DARKMAGENTA OR PURPLE
    case 5: return 0x3E03; // GREEN
    case 6: return 0x0015; // BLUE
    case 7: return YELLOW;
    case 8: return ORANGE; // ORANGE
    case 9: return 0xFFFA; // PALE YELLOW
    case 10: return 0xFC10; // PINK OR LT RED
    case 11: return 0xB73F; // LIGHTBLUE
    case 12: return 0xF61F; // LIGHTMAGENTA
    case 13: return 0xBFF7; // LIGHTGREEN
    case 14: return 0xB73F; // LIGHTBLUE
    case 15: return 0xDF50; // YELLOW BROWN
  #endif    
#ifdef ARDUINO_TEENSY41
    case 0: return 0x0000; // BLACK
    case 1: return 0xFFFF; // WHITE
    case 2: return 0x8800; // RED
    case 3: return 0xA77E; // CYAN
    case 4: return 0x8118; // DARKMAGENTA OR PURPLE
    case 5: return 0x3E03; // GREEN
    case 6: return 0x0015; // BLUE
#ifdef ILI9341
    case 7: return ILI9341_YELLOW;
    case 8: return ILI9341_ORANGE;
#endif    
#ifdef ILI9488
    case 7: return ILI9488_YELLOW;
    case 8: return ILI9488_ORANGE;
#endif    
    case 9: return 0xFFFA; // PALE YELLOW
    case 10: return 0xFC10; // PINK OR LT RED
    case 11: return 0xB73F; // LIGHTBLUE
    case 12: return 0xF61F; // LIGHTMAGENTA
    case 13: return 0xBFF7; // LIGHTGREEN
    case 14: return 0xB73F; // LIGHTBLUE
    case 15: return 0xDF50; // YELLOW BROWN
  #endif    
  #ifdef ARDUINO_LILYGO_T_DISPLAY_S3
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
#endif
    default: return 0;
  }
}

static int average_color(int color1, int color2)
{
  if (color1 == color2)
    return color1;    
  // extract 565 color from 16-bit values, average them, and combine back the same way
  int red = ((color1 >> 11) + (color2 >> 11)) / 2;
  int green = (((color1 >> 5) & 0x3F) + (((color2 >> 5) & 0x3F))) / 2;
  int blue = ((color1 & 0x1F) + (color2 & 0x1F)) / 2;
  int color = ((red & 0x1f) << 11) | ((green & 0x3f) << 5) | (blue & 0x1F);
  return color;
}

void EmuVic::DrawChar(byte c, int col, int row, int fg, int bg)
{
  if (postponeDrawChar)
    return;

#ifdef M5STACK
  M5.Lcd.startWrite();
#endif
  int offset = ((io[0x5] & 2) == 0) ? 0 : (8*256);
  const byte* shape = &chargen[c*8+offset];
#ifdef M5STACK  
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
#endif
#ifdef ARDUINO_SUNTON_8048S070
  int y0 = Y0 + row*8*2;
  int x0 = X0 + col*8*4;
  int maxcol = 8;
#endif
#ifdef ARDUINO_TEENSY41
 #ifdef ILI9341
  int y0 = Y0 + row*8;
  int x0 = X0 + col*12;
  int midcolor = bg;
  int maxcol = 12;
  int last_color;
 #endif    
 #ifdef ILI9488
  int y0 = Y0 + row*12;
  int x0 = X0 + col*20;
  int maxcol = 8;
  #define SCALEX(n) ((n*5+1)/2)
  #define SCALEY(n) ((n*3+1)/2)
  int colors[8][8];
 #endif
#endif
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
  int maxcol = 8;
  int y0 = Y0 + row*7;
  int x0 = X0 + col*14;
  int scalex[] = { 0, 2, 4, 6, 7, 9, 11, 13 };
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
          color = average_color(color, last_color);
      }
      else
      {
        mask = mask >> 1;
        last_color = color;
      }
  #endif
#ifdef M5STACK  
      M5.Lcd.drawPixel(x0+col_i, y0+row_i, color);
#endif
#ifdef ARDUINO_SUNTON_8048S070
      int x1=x0+col_i*4;
      int y1=y0+row_i*2;
      gfx->drawPixel(x1, y1, color);
      gfx->drawPixel(x1+1, y1, color);
      gfx->drawPixel(x1+2, y1, color);
      gfx->drawPixel(x1+3, y1, color);
      gfx->drawPixel(x1, y1+1, color);
      gfx->drawPixel(x1+1, y1+1, color);
      gfx->drawPixel(x1+2, y1+1, color);
      gfx->drawPixel(x1+3, y1+1, color);
#endif
#ifdef ILI9341
      lcd.drawPixel(y0+row_i, 319-(x0+col_i), color);
#endif
#ifdef ILI9488
      lcd.drawPixel(y0+SCALEY(row_i), 479-(x0+SCALEX(col_i)), color);
      lcd.drawPixel(y0+SCALEY(row_i), 479-(x0+SCALEX(col_i)+1), color);
      if (col_i & 1) {
        int color2 = average_color(color, colors[row_i][col_i-1]);
        lcd.drawPixel(y0+SCALEY(row_i), 479-(x0+SCALEX(col_i)-1), color2);
      }
      if ((row_i % 2) == 1)
      {
        int color3 = average_color(color, colors[row_i-1][col_i]);
        lcd.drawPixel(y0+SCALEY(row_i)-1, 479-(x0+SCALEX(col_i)), color3);
        lcd.drawPixel(y0+SCALEY(row_i)-1, 479-(x0+SCALEX(col_i)+1), color3);
        if (col_i & 1)
        {
          int color2 = average_color(colors[row_i][col_i-1], colors[row_i-1][col_i-1]);
          lcd.drawPixel(y0+SCALEY(row_i)-1, 479-(x0+SCALEX(col_i)-1), average_color(color2, color3));
        }
      }
      colors[row_i][col_i] = color;
#endif
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
      if (row_i < 6)
      {
        lcd.drawPixel(x0+scalex[col_i], y0+row_i, color);
        if (col_i < 7 && col_i != 3)
        {
          int color2 = ((shape[row_i] & (mask >> 1)) == 0) ? bg : fg;
          lcd.drawPixel(x0+scalex[col_i]+1, y0+row_i, average_color(color, color2));
        }
      }
      else if (row_i == 6)
      {
        int color2 = ((shape[row_i+1] & mask) == 0) ? bg : fg;
        lcd.drawPixel(x0+scalex[col_i], y0+row_i, average_color(color, color2));
        if (col_i < 7 && col_i != 3)
        {
          int color3 = ((shape[row_i] & (mask >> 1)) == 0) ? bg : fg;
          int color4 = ((shape[row_i+1] & (mask >> 1)) == 0) ? bg : fg;
          lcd.drawPixel(x0+scalex[col_i]+1, y0+row_i, average_color(color2, average_color(color3, color4)));
        }
      }
      mask = mask >> 1;
#endif
    }
  }
#ifdef M5STACK
  M5.Lcd.endWrite();
#endif
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
#ifdef M5STACK
  M5.Lcd.startWrite();
#endif
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
#ifdef M5STACK
  M5.Lcd.endWrite();
#endif  
}

void EmuVic::RedrawScreenEfficientlyAfterPostponed()
{
#ifdef M5STACK
  M5.Lcd.startWrite();
#endif  
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
#ifdef M5STACK
  M5.Lcd.endWrite();
#endif  
}

void EmuVic::DrawBorder(byte value)
{
    int border = Vic20ColorToLCDColor(value & 7);
#ifdef M5STACK
    M5.Lcd.startWrite();
    M5.Lcd.fillRect(0, 0, 320, Y0, border);
    M5.Lcd.fillRect(0, Y0, X0, 23*8, border);
    M5.Lcd.fillRect(320-X0, Y0, X0, 23*8, border);
    M5.Lcd.fillRect(0, 240-Y0, 320, Y0, border);
    M5.Lcd.endWrite();
#endif
#ifdef ARDUINO_SUNTON_8048S070
    gfx->fillRect(0, 0, 800, Y0, border);
    gfx->fillRect(0, Y0, X0, 23*8*4, border);
    gfx->fillRect(800-X0, Y0, X0, 23*8*4, border);
    gfx->fillRect(0, 480-Y0, 800, Y0, border);
#endif
#ifdef ARDUINO_TEENSY41
#ifdef ILI9341
    lcd.fillRect(0, 0, Y0, 320, border);
    lcd.fillRect(Y0, 0, 23*8, X0, border);
    lcd.fillRect(Y0, 320-X0, 23*8, X0, border);
    lcd.fillRect(240-Y0, 0, Y0, 320, border);
#endif
#ifdef ILI9488    
    lcd.fillRect(0, 480-X0, 320, X0, border);
    lcd.fillRect(0, X0, Y0, 480-X0*2, border);
    lcd.fillRect(0, 0, 320, X0+1, border);
    lcd.fillRect(320-Y0, X0, Y0, 480-X0*2, border);
#endif
#endif
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    lcd.fillRect(0, 0, 320, Y0, border);
    lcd.fillRect(0, Y0, X0, 23*7, border);
    lcd.fillRect(320-X0, Y0, X0, 23*7, border);
    lcd.fillRect(0, 170-Y0-1, 320, Y0-1, border);
#endif
}

void EmuVic::SaveOldVideoAndColor()
{
    memcpy(&old_video[0], &ram[0x1E00], 22*23);
    memcpy(&old_color[0], &color_nybles[0], 22*23);
}