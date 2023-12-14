#include "vicii.h"
#include "config.h"

EmuVicII::EmuVicII(byte* vram, byte* vio, byte* vcolor_nybles, byte* vchargen)
{
  ram = vram;
  io = vio;
  color_nybles = vcolor_nybles;
  chargen = vchargen;
  old_video = new byte[1000];
  old_color = new byte[1000];
  postponeDrawChar = false;

  memset(old_video, 32, 1000);
  memset(old_color, 0, 1000);

  // initialize LCD screen
#ifdef M5STACK
  M5.Lcd.fillScreen(0x0000);  // BLACK
#endif
#ifdef ARDUINO_SUNTON_8048S070
  gfx->fillScreen(0x0000);  // BLACK
#endif
}

EmuVicII::~EmuVicII()
{
  delete [] old_video;
  delete [] old_color;
}

 // RGB565 colors picked with http://www.barth-dev.de/online/rgb565-color-picker/
int EmuVicII::C64ColorToLCDColor(byte value)
{
  switch (value & 0xF)
  {
#ifdef M5STACK    
    case 0: return TFT_BLACK;
    case 1: return TFT_WHITE;
    case 2: return TFT_RED;
    case 3: return TFT_CYAN;
    case 4: return 0x8118; // DARKMAGENTA OR PURPLE
    case 5: return 0x0400; // DARKGREEN
    case 6: return TFT_BLUE;
    case 7: return TFT_YELLOW;
    case 8: return TFT_ORANGE;
    case 9: return 0x8283; // BROWN;
    case 10: return 0xFC10; // PINK OR LT RED
    case 11: return TFT_DARKGREY;
    case 12: return TFT_DARKCYAN; // MED GRAY
    case 13: return 0xBFF7; // LIGHTGREEN
    case 14: return 0x841F; // LIGHTBLUE
    case 15: return TFT_LIGHTGREY;
#endif
#ifdef ARDUINO_SUNTON_8048S070
    case 0: return BLACK;
    case 1: return WHITE;
    case 2: return RED;
    case 3: return CYAN;
    case 4: return 0x8118; // DARKMAGENTA OR PURPLE
    case 5: return 0x0400; // DARKGREEN
    case 6: return BLUE;
    case 7: return YELLOW;
    case 8: return ORANGE;
    case 9: return 0x8283; // BROWN;
    case 10: return 0xFC10; // PINK OR LT RED
    case 11: return DARKGREY;
    case 12: return DARKCYAN; // MED GRAY
    case 13: return 0xBFF7; // LIGHTGREEN
    case 14: return 0x841F; // LIGHTBLUE
    case 15: return LIGHTGREY;
#endif    
#ifdef ILI9341    
    case 0: return ILI9341_BLACK;
    case 1: return ILI9341_WHITE;
    case 2: return ILI9341_RED;
    case 3: return ILI9341_CYAN;
    case 4: return 0x8118; // DARKMAGENTA OR PURPLE
    case 5: return 0x0400; // DARKGREEN
    case 6: return ILI9341_BLUE;
    case 7: return ILI9341_YELLOW;
    case 8: return ILI9341_ORANGE;
    case 9: return 0x8283; // BROWN;
    case 10: return 0xFC10; // PINK OR LT RED
    case 11: return ILI9341_DARKGREY;
    case 12: return ILI9341_DARKCYAN; // MED GRAY
    case 13: return 0xBFF7; // LIGHTGREEN
    case 14: return 0x841F; // LIGHTBLUE
    case 15: return ILI9341_LIGHTGREY;
#endif    
#ifdef ILI9488    
    case 0: return ILI9488_BLACK;
    case 1: return ILI9488_WHITE;
    case 2: return ILI9488_RED;
    case 3: return ILI9488_CYAN;
    case 4: return 0x8118; // DARKMAGENTA OR PURPLE
    case 5: return 0x0400; // DARKGREEN
    case 6: return ILI9488_BLUE;
    case 7: return ILI9488_YELLOW;
    case 8: return ILI9488_ORANGE;
    case 9: return 0x8283; // BROWN;
    case 10: return 0xFC10; // PINK OR LT RED
    case 11: return ILI9488_DARKGREY;
    case 12: return ILI9488_DARKCYAN; // MED GRAY
    case 13: return 0xBFF7; // LIGHTGREEN
    case 14: return 0x841F; // LIGHTBLUE
    case 15: return ILI9488_LIGHTGREY;
#endif
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    case 0: return TFT_BLACK;
    case 1: return TFT_WHITE;
    case 2: return TFT_RED;
    case 3: return TFT_CYAN;
    case 4: return 0x8118; // DARKMAGENTA OR PURPLE
    case 5: return 0x0400; // DARKGREEN
    case 6: return TFT_BLUE;
    case 7: return TFT_YELLOW;
    case 8: return TFT_ORANGE;
    case 9: return 0x8283; // BROWN;
    case 10: return 0xFC10; // PINK OR LT RED
    case 11: return TFT_DARKGREY;
    case 12: return TFT_DARKCYAN; // MED GRAY
    case 13: return 0xBFF7; // LIGHTGREEN
    case 14: return 0x841F; // LIGHTBLUE;
    case 15: return TFT_LIGHTGREY;
#endif
    default: return 0;
  }
}

#ifdef ILI9488  
int static scale_index(int i)
{
  return (i*3+1)/2;
}
int static average_color(int color0, int color2)
{
  if (color0 == color2)
    return color0;  
  unsigned char r0, g0, b0, r1, g1, b1, r2, g2, b2;
  lcd.color565toRGB(color0, r0, g0, b0);
  lcd.color565toRGB(color2, r2, g2, b2);
  r1 = (r0+r2)/2;
  g1 = (g0+g2)/2;
  b1 = (b0+b2)/2;
  int color1 = CL(r1, g1, b1);
  return color1;
}
int static average_color(int color0, int color2, int color3, int color4)
{
  if (color0 == color2 && color2 == color3 && color3 == color4)
    return color0;  
  unsigned char r0, g0, b0, r1, g1, b1, r2, g2, b2, r3, g3, b3, r4, g4, b4;
  lcd.color565toRGB(color0, r0, g0, b0);
  lcd.color565toRGB(color2, r2, g2, b2);
  lcd.color565toRGB(color3, r3, g3, b3);
  lcd.color565toRGB(color4, r4, g4, b4);
  r1 = (r0+r2+r3+r4)/4;
  g1 = (g0+g2+g3+g4)/4;
  b1 = (b0+b2+b3+b4)/4;
  int color1 = CL(r1, g1, b1);
  return color1;
}
#endif
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
static int average_color(int color1, int color2)
{ // extract 565 color from 16-bit values, average them, and combine back the same way
  int red = ((color1 >> 11) + (color2 >> 11)) / 2;
  int green = (((color1 >> 5) & 0x3F) + (((color2 >> 5) & 0x3F))) / 2;
  int blue = ((color1 & 0x1F) + (color2 & 0x1F)) / 2;
  return ((red & 0x1f) << 11) | ((green & 0x3f) << 5) | (blue & 0x1F);
}
#endif

void EmuVicII::DrawChar(byte c, int col, int row, int fg, int bg)
{
  if (postponeDrawChar)
    return;
  
#ifdef M5STACK  
  M5.Lcd.startWrite();
#endif  
  int offset = ((io[0x18] & 2) == 0) ? 0 : (8*256);
  const byte* shape = &chargen[c*8+offset];
#ifdef M5STACK  
  int x0 = 0 + col*8;
  int y0 = 20 + row*8;
#endif
#ifdef ARDUINO_SUNTON_8048S070
  int x0 = 80 + col*16;
  int y0 = 40 + row*16;
#endif
#ifdef ILI9341  
  int x0 = 20 + row*8;
  int y0 = col*8;
#endif  
#ifdef ILI9488  
  int x0 = 10 + row*12;
  int y0 = col*12;
#endif
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
  int x0 = 0 + col*8;
  int y0 = 10 + row*6;
#endif  
for (int row_i=0; row_i<8; ++row_i)
  {
    int mask = 128;
    for (int col_i=0; col_i<8; ++col_i)
    {
      int color = ((shape[row_i] & mask) == 0) ? bg : fg;
#ifdef M5STACK      
      M5.Lcd.drawPixel(x0+col_i, y0+row_i, color);
#endif
#ifdef ARDUINO_SUNTON_8048S070
      gfx->drawPixel(x0+col_i*2, y0+row_i*2, color);
      gfx->drawPixel(x0+col_i*2+1, y0+row_i*2, color);
      gfx->drawPixel(x0+col_i*2, y0+row_i*2+1, color);
      gfx->drawPixel(x0+col_i*2+1, y0+row_i*2+1, color);
#endif
#ifdef ILI9341
      lcd.drawPixel(x0+col_i, y0+row_i, color);
#endif
#ifdef ILI9488
      lcd.drawPixel(x0+scale_index(row_i), 479 - (y0+scale_index(col_i)), color);
      if ((col_i % 2) == 0)
      {
        int color2 = ((shape[row_i] & (mask >> 1)) == 0) ? bg : fg;
        lcd.drawPixel(x0+scale_index(row_i), 479 - (y0+scale_index(col_i)+1), average_color(color, color2));
      }
      if ((row_i % 2) == 0)
      {
        int color2 = ((shape[row_i+1] & mask) == 0) ? bg : fg;
        lcd.drawPixel(x0+scale_index(row_i)+1, 479 - (y0+scale_index(col_i)), average_color(color, color2));
      }
      if ((col_i % 2) == 0 && (row_i % 2) == 0)
      {
        int color2 = ((shape[row_i] & (mask >> 1)) == 0) ? bg : fg;
        int color3 = ((shape[row_i+1] & mask) == 0) ? bg : fg;
        int color4 = ((shape[row_i+1] & (mask >> 1)) == 0) ? bg : fg;
        lcd.drawPixel(x0+scale_index(row_i)+1, 479 - (y0+scale_index(col_i)+1), average_color(color, color2, color3, color4));
      }
#endif
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
      if (row_i == 0) // color average the top two rows of character
        lcd.drawPixel(x0+col_i, y0+row_i, (((shape[0] | shape[1]) & mask) == 0) ? bg : (((shape[0] & mask) == (shape[1] & mask)) ? fg : average_color(fg, bg)));
      else if (row_i == 5) // color average the bottom two rows of character
        lcd.drawPixel(x0+col_i, y0+row_i, (((shape[6] | shape[7]) & mask) == 0) ? bg : (((shape[6] & mask) == (shape[7] & mask)) ? fg : average_color(fg, bg)));
      else if (row_i < 5) // keep detail of four center rows of character
        lcd.drawPixel(x0+col_i, y0+row_i, ((shape[row_i+1] & mask) == 0) ? bg : fg);
#endif
      mask = mask >> 1;
    }
  }
#ifdef M5STACK  
  M5.Lcd.endWrite();
#endif  
}

void EmuVicII::DrawChar(int offset)
{
  int col = offset % 40;
  int row = offset / 40;
  int fg = EmuVicII::C64ColorToLCDColor(color_nybles[offset]);
  int bg = EmuVicII::C64ColorToLCDColor(io[0x21]);
  DrawChar(ram[1024+offset], col, row, fg, bg);
}

void EmuVicII::RedrawScreen()
{
#ifdef M5STACK   
  M5.Lcd.startWrite();
#endif  
  int bg = EmuVicII::C64ColorToLCDColor(io[0x21]);
  int offset = 0;
  for (int row = 0; row < 25; ++row)
  {
    for (int col = 0; col < 40; ++col)
    {
      int fg = EmuVicII::C64ColorToLCDColor(color_nybles[offset]);
      DrawChar(ram[1024 + offset], col, row, fg, bg);
      ++offset;
    }
  }
#ifdef M5STACK  
  M5.Lcd.endWrite();
#endif  
}

void EmuVicII::RedrawScreenEfficientlyAfterPostponed()
{
#ifdef M5STACK
  M5.Lcd.startWrite();
#endif  
  int bg = EmuVicII::C64ColorToLCDColor(io[0x21]);
  int offset = 0;
  for (int row = 0; row < 25; ++row)
  {
    for (int col = 0; col < 40; ++col)
    {
      int fg = EmuVicII::C64ColorToLCDColor(color_nybles[offset]);
      byte old_char = old_video[offset];
      byte new_char = ram[1024 + offset];
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

void EmuVicII::DrawBorder(byte value)
{
    int border = C64ColorToLCDColor(value);
#ifdef M5STACK
    M5.Lcd.startWrite();
    M5.Lcd.fillRect(0, 0, 320, 20, border);
    M5.Lcd.fillRect(0, 220, 320, 20, border);
    M5.Lcd.endWrite();
#endif    
#ifdef ARDUINO_SUNTON_8048S070
    gfx->fillRect(0, 0, 800, 40, border);
    gfx->fillRect(0, 440, 800, 40, border);
    gfx->fillRect(0, 40, 80, 400, border);
    gfx->fillRect(720, 40, 80, 400, border);
#endif
#ifdef ILI9341    
    lcd.fillRect(0, 0, 20, 320, border);
    lcd.fillRect(220, 0, 20, 320, border);
#endif    
#ifdef ILI9488    
    lcd.fillRect(0, 0, 10, 480, border);
    lcd.fillRect(310, 0, 10, 480, border);
#endif   
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    lcd.fillRect(0, 0, 320, 10, border);
    lcd.fillRect(0, 160, 320, 10, border);
#endif
}

void EmuVicII::SaveOldVideoAndColor()
{
    memcpy(&old_video[0], &ram[1024], 1000);
    memcpy(&old_color[0], &color_nybles[0], 1000);
}