// VDC8563 - 80 column video display chip on C128 (and VDC8568 on C128D)
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

#include "emu6502.h"
#include "config.h"
#include "vdc.h"

#ifdef _WINDOWS
#include <memory.h>
#include <WindowsDraw.h>
#include <WindowsTime.h>
#endif // _WINDOWS

const int vdc_ram_size = 64 * 1024;
const int registers_size = 38;

VDC8563::VDC8563()
{
    registers = new byte[registers_size]
    {
        126, 80, 102, 73, 32, 224, 25, 29,
        252, 231, 160, 231, 0, 0, 0, 0,
        0, 0, 15, 228, 8, 0, 120, 232,
        32, 71, 240, 0, 63, 21, 79, 0,
        0, 0, 125, 100, 245, 63
    };

    vdc_ram = new byte[vdc_ram_size];
    memset(vdc_ram, 0, vdc_ram_size);
}

VDC8563::~VDC8563()
{
    delete[] registers;
    delete[] vdc_ram;
}

void VDC8563::Activate()
{
  if (!active)
  {
    active = true;
    int bg = VDCColorToLCDColor(registers[26]);
#ifdef _WINDOWS
    if (!WindowsDraw::CreateRenderTarget(640, 200, 32, 16, redrawRequiredSignal))
        throw "CreateRenderTarget failed";
    WindowsDraw::BeginDraw();
    WindowsDraw::DrawBorder(0, 0, 0); // BLACK
    unsigned long last_refresh = micros();
    needsPaintFrame = false;
#endif
#ifdef ARDUINO_SUNTON_8048S070
    gfx->fillRect(0, 0, 800, 480, bg);
#endif
#ifdef M5STACK
    M5.Lcd.fillRect(0, 0, 320, 240, bg);
#endif
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    lcd.fillRect(0, 0, 320, 170, bg);
#endif
#ifdef ILI9341
    lcd.fillRect(0, 0, 240, 320, bg);
#endif
#ifdef ILI9488
    lcd.fillRect(0, 0, 320, 480, bg);
#endif
    RedrawScreen();
  }
}

void VDC8563::Deactivate()
{
  active = false;
#ifdef _WINDOWS
  WindowsDraw::EndDraw();
#endif
}

int VDC8563::VDCColorToLCDColor(byte value)
{
  switch (value & 15)
  {
    case 0: return 0x0000; // BLACK
#ifdef ARDUINO_SUNTON_8048S070
    case 1: return DARKGREY;
    case 2: return BLUE;
    case 3: return 0x841F; // LIGHTBLUE
    case 4: return 0x0400; // DARKGREEN
    case 5: return 0xBFF7; // LIGHTGREEN
    case 6: return DARKCYAN; // MED GRAY
    case 7: return CYAN;
    case 8: return RED;
    case 9: return 0xFC10; // PINK OR LT RED
    case 10: return 0x8118; // DARKMAGENTA OR PURPLE
    case 11: return 0xF81F; // LIGHT MAGENTA
    case 12: return 0x8283; // BROWN;
    case 13: return YELLOW;
    case 14: return LIGHTGREY;
    case 15: return WHITE;
#endif
#ifdef M5STACK
    case 1: return TFT_DARKGREY;
    case 2: return TFT_BLUE;
    case 3: return 0x841F; // LIGHTBLUE
    case 4: return 0x0400; // DARKGREEN
    case 5: return 0xBFF7; // LIGHTGREEN
    case 6: return TFT_DARKCYAN; // MED GRAY
    case 7: return TFT_CYAN;
    case 8: return TFT_RED;
    case 9: return 0xFC10; // PINK OR LT RED
    case 10: return 0x8118; // DARKMAGENTA OR PURPLE
    case 11: return 0xF81F; // LIGHT MAGENTA
    case 12: return 0x8283; // BROWN;
    case 13: return TFT_YELLOW;
    case 14: return TFT_LIGHTGREY;
    case 15: return TFT_WHITE;
#endif
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    case 1: return TFT_DARKGREY;
    case 2: return TFT_BLUE;
    case 3: return 0x841F; // LIGHTBLUE
    case 4: return 0x0400; // DARKGREEN
    case 5: return 0xBFF7; // LIGHTGREEN
    case 6: return TFT_DARKCYAN; // MED GRAY
    case 7: return TFT_CYAN;
    case 8: return TFT_RED;
    case 9: return 0xFC10; // PINK OR LT RED
    case 10: return 0x8118; // DARKMAGENTA OR PURPLE
    case 11: return 0xF81F; // LIGHT MAGENTA
    case 12: return 0x8283; // BROWN;
    case 13: return TFT_YELLOW;
    case 14: return TFT_LIGHTGREY;
    case 15: return TFT_WHITE;
#endif
#ifdef ILI9341
    case 1: return ILI9341_DARKGREY;
    case 2: return ILI9341_BLUE;
    case 3: return 0x841F; // LIGHTBLUE
    case 4: return 0x0400; // DARKGREEN
    case 5: return 0xBFF7; // LIGHTGREEN
    case 6: return ILI9341_DARKCYAN; // MED GRAY
    case 7: return ILI9341_CYAN;
    case 8: return ILI9341_RED;
    case 9: return 0xFC10; // PINK OR LT RED
    case 10: return 0x8118; // DARKMAGENTA OR PURPLE
    case 11: return 0xF81F; // LIGHT MAGENTA
    case 12: return 0x8283; // BROWN;
    case 13: return ILI9341_YELLOW;
    case 14: return ILI9341_LIGHTGREY;
    case 15: return ILI9341_WHITE;
#endif
#ifdef ILI9488    
    case 1: return ILI9488_DARKGREY;
    case 2: return ILI9488_BLUE;
    case 3: return 0x841F; // LIGHTBLUE
    case 4: return 0x0400; // DARKGREEN
    case 5: return 0xBFF7; // LIGHTGREEN
    case 6: return ILI9488_DARKCYAN; // MED GRAY
    case 7: return ILI9488_CYAN;
    case 8: return ILI9488_RED;
    case 9: return 0xFC10; // PINK OR LT RED
    case 10: return 0x8118; // DARKMAGENTA OR PURPLE
    case 11: return 0xF81F; // LIGHT MAGENTA
    case 12: return 0x8283; // BROWN;
    case 13: return ILI9488_YELLOW;
    case 14: return ILI9488_LIGHTGREY;
    case 15: return ILI9488_WHITE;
#endif  
#ifdef _WINDOWS
    case 1: return 0x8410; // DARKGRAY
    case 2: return 0x0018; // BLUE
    case 3: return 0x841F; // LIGHTBLUE
    case 4: return 0x0400; // DARKGREEN
    case 5: return 0xBFF7; // LIGHTGREEN
    case 6: return 0x0208; // MED GRAY OR DARK CYAN
    case 7: return 0x07FF; // CYAN
    case 8: return 0xF800; // RED
    case 9: return 0xFC10; // PINK OR LT RED
    case 10: return 0x8118; // DARKMAGENTA OR PURPLE
    case 11: return 0xF81F; // LIGHT MAGENTA
    case 12: return 0x8283; // BROWN;
    case 13: return 0xFFE0; // YELLOW;
    case 14: return 0x4208; // LIGHTGREY
    case 15: return 0xFFFF; // WHITE
#endif  
    default: return 0xFFFF; // WHITE
  }
}

#ifdef M5STACK
static int average_color(int color1, int color2)
{ // extract 565 color from 16-bit values, average them, and combine back the same way
  int red = ((color1 >> 11) + (color2 >> 11)) / 2;
  int green = (((color1 >> 5) & 0x3F) + (((color2 >> 5) & 0x3F))) / 2;
  int blue = ((color1 & 0x1F) + (color2 & 0x1F)) / 2;
  return ((red & 0x1f) << 11) | ((green & 0x3f) << 5) | (blue & 0x1F);
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
#ifdef ILI9341
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
#endif
#ifdef ILI9488
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
#endif

#ifdef _WINDOWS
static void Extract565Color(int color, byte& red, byte& green, byte& blue) // TODO: one copy
{
    red = (color >> 11) * 255 / 32;
    green = ((color >> 5) & 0x3F) * 255 / 64;
    blue = (color & 0x1F) * 255 / 32;
}
#endif

void VDC8563::DrawChar(byte c, int col, int row, int fg, int bg, byte attrib)
{
  if (!active)
    return;

#ifdef ARDUINO_SUNTON_8048S070      
  int x0 = 80 + col * 8;
  int y0 = 40 + row * 16;
#endif
#ifdef M5STACK
  int x0 = col * 4;
  int y0 = 20 + row * 8;
  int colors[8];
  M5.Lcd.startWrite();
#endif
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
  int x0 = col * 4;
  int y0 = 10 + row * 6;
  int colors[4];
  int last_color;
#endif
#ifdef ILI9341
  int x0 = col * 4;
  int y0 = 20 + row * 8;
  int colors[8];
#endif
#ifdef ILI9488
  int x0 = 10 + row * 12;
  int y0 = col * 6;
  #define SCALEY(n) ((n*3+1)/2)
  int colors[8];
  bool fill = true;
#endif
  byte* src = &vdc_ram[0x2000] + 16 * c + 4096 * (attrib >> 7);
  bool inverse = ((registers[24] & 0x40) != 0);
  byte cursorMode = (registers[10] >> 5) & 3;
  bool isCursor = ( row*80+col == (registers[14] << 8 | registers[15]) && cursorMode != 1 );
  if (attrib & 0x20 && registers[29] < 8 && !(inverse ^ isCursor)) {
      static byte underlined[8];
      memcpy(underlined, src, 8);
      underlined[registers[29]] = 255;
      src = underlined;
  }
#ifdef _WINDOWS
  byte fg_red, fg_green, fg_blue;
  byte bg_red, bg_green, bg_blue;
  Extract565Color(fg, fg_red, fg_green, fg_blue);
  Extract565Color(bg, bg_red, bg_green, bg_blue);
  if (inverse ^ isCursor)
	  WindowsDraw::DrawCharacter2Color(src, col, row, bg_red, bg_green, bg_blue, fg_red, fg_green, fg_blue);
  else
	  WindowsDraw::DrawCharacter2Color(src, col, row, fg_red, fg_green, fg_blue, bg_red, bg_green, bg_blue);
  needsPaintFrame = true;
  return;
#else // NOT _WINDOWS
  for (int row_i=0; row_i<8; ++row_i)
  {
    int mask = 128;
    bool underlined = ( ( (attrib & 0x20) != 0 ) && ( row_i == (registers[29] & 0x1F) ) );
    for (int col_i=0; col_i<8; ++col_i)
    {
      int color = (((src[row_i] & mask) != 0 || underlined) ^ inverse ^ isCursor) ? fg : bg;
#ifdef ARDUINO_SUNTON_8048S070      
      gfx->drawPixel(x0+col_i, y0+row_i*2, color);
      gfx->drawPixel(x0+col_i, y0+row_i*2+1, color);
#endif      
#ifdef M5STACK
      if (col_i & 1)
        M5.Lcd.drawPixel(x0+col_i/2, y0+row_i, average_color(colors[col_i-1], color));
      colors[col_i] = color;
#endif
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
      if (col_i & 1)
      {
        int mid = average_color(last_color, color);
        if (row_i > 1 && row_i < 6)
          lcd.drawPixel(x0+col_i/2, y0+row_i-1, mid);
        else if (row_i == 1 || row_i == 7)
        {
          int adj = (row_i == 7) ? 2 : 1;
          mid = average_color(mid, colors[col_i/2]);
          lcd.drawPixel(x0+col_i/2, y0+row_i-adj, mid);
        }
        else
          colors[col_i/2] = mid;
      }
      else
        last_color = color;
#endif
#ifdef ILI9341
      if (col_i & 1)
        lcd.drawPixel(y0+row_i, 319-(x0+col_i/2), average_color(colors[col_i-1], color));
      colors[col_i] = color;
#endif
#ifdef ILI9488
      if (col_i >= 2 && col_i <= 5)
      {
        if (fill && (row_i & 1) == 1)
          lcd.drawPixel(x0+SCALEY(row_i)-1, 479-(y0+col_i-1), average_color(color, colors[col_i]));
        lcd.drawPixel(x0+SCALEY(row_i), 479-(y0+col_i-1), color);
      }
      else if (col_i == 1 || col_i == 7)
      {
        int adj = (col_i == 7) ? 1 : 0;
        int mid = average_color(color, colors[col_i-1]);
        if (fill && (row_i & 1) == 1)
        {
          int midabove = average_color(colors[col_i], (((src[row_i-1] & (mask << 1)) != 0 || underlined) ^ inverse ^ isCursor) ? fg : bg);
          lcd.drawPixel(x0+SCALEY(row_i)-1, 479-(y0+col_i-1-adj), average_color(mid, midabove));
        }
        lcd.drawPixel(x0+SCALEY(row_i), 479-(y0+col_i-1-adj), mid);
      }
      colors[col_i] = color;
#endif
      mask >>= 1;
    }
  }
#ifdef M5STACK   
  M5.Lcd.endWrite();
#endif
#endif // NOT _WINDOWS
}

void VDC8563::DrawChar(int offset)
{
  int col = offset % 80;
  int row = offset / 80;
  bool attributesEnabled = ((registers[25] & 0x40) != 0);
  byte attrib = vdc_ram[2048+offset];
  int fg = attributesEnabled ? VDCColorToLCDColor(attrib) : VDCColorToLCDColor(registers[26] >> 4);
  int bg = VDCColorToLCDColor(registers[26]);
  DrawChar(vdc_ram[offset], col, row, fg, bg, vdc_ram[offset+0x800]);
}

void VDC8563::RedrawScreen()
{
  if (!active)
    return;

#ifdef M5STACK   
  M5.Lcd.startWrite();
#endif  
  int bg = VDCColorToLCDColor(registers[26]);
  int offset = 0;
  bool attributesEnabled = ((registers[25] & 0x40) != 0);
  for (int row = 0; row < 25; ++row)
  {
    for (int col = 0; col < 80; ++col)
    {
      byte attrib = vdc_ram[2048+offset];
      int fg = attributesEnabled ? VDCColorToLCDColor(attrib) : VDCColorToLCDColor(registers[26] >> 4);
      DrawChar(vdc_ram[offset], col, row, fg, bg, attrib); // TODO: optimize whether need to draw
      ++offset;
    }
  }
#ifdef M5STACK  
  M5.Lcd.endWrite();
#endif  
}

byte VDC8563::GetAddressRegister()
{
    if (ready)
    {
        return 128;
    }
    else
    {
        ready = true; // simulate delay in processing
        return 0;
    }
}

void VDC8563::SetAddressRegister(byte value)
{
    register_addr = value & 0x3F;
    if (register_addr < registers_size)
        data = registers[register_addr];
    else
        data = 0xFF;
    ready = false; // simulate delay in processing
}

byte VDC8563::GetDataRegister()
{
    if (ready)
    {
        if (register_addr == 31)
        {
            ushort dest = (ushort)((registers[18] << 8) + registers[19]);
            data = vdc_ram[dest++];
            registers[18] = (byte)(dest >> 8);
            registers[19] = (byte)dest;
        }

        return data;
    }
    else
    {
        ready = true;
        return 0xFF;
    }
}

void VDC8563::SetDataRegister(byte value)
{
    ready = true;
    if (register_addr >= registers_size)
      return;
    if (register_addr == 31)
    {
        registers[register_addr] = value;
        ushort dest = (ushort)((registers[18] << 8) + registers[19]);
        if (vdc_ram[dest] != value) // optimize drawing
        {
          vdc_ram[dest] = value;
          if (active)
          {
            if (dest >= 0 && dest < 2000)
              DrawChar(dest);
            else if (dest >= 2048 && dest < 4048)
              DrawChar(dest & 0x7FF);
          }
        }
        ++dest;
        registers[18] = (byte)(dest >> 8);
        registers[19] = (byte)dest;
    }
    else if (register_addr == 30)
    {
        registers[register_addr] = value;
        int count = (value == 0) ? 256 : value;
        ushort dest = (ushort)((registers[18] << 8) + registers[19]);
        bool fill = ((registers[24] & 0x80) == 0);
        bool copy = !fill;
        if (fill)
        {
            for (int i = 0; i < count; ++i)
            {
                if (vdc_ram[dest] != registers[31]) { // optimize out redundant screen updates
                    vdc_ram[dest] = registers[31];
                    if (active)
                    {
                        if (dest >= 0 && dest < 2000)
                            DrawChar(dest);
                        else if (dest >= 2048 && dest < 4048)
                            DrawChar(dest & 0x7FF);
                    }
                }
                ++dest;
            }
        }
        else if (copy)
        {
            ushort src = (ushort)((registers[32] << 8) + registers[33]);
            for (int i = 0; i < count; ++i)
            {
                if (vdc_ram[dest] != vdc_ram[src]) // optimize drawing
                {
                  vdc_ram[dest] = vdc_ram[src];
                  if (active)
                  {
                    if (dest >= 0 && dest < 2000)
                      DrawChar(dest);
                    else if (dest >= 2048 && dest < 4048)
                      DrawChar(dest & 0x7FF);
                  }
                }
                ++dest;
                ++src;
            }
            registers[32] = (byte)(src >> 8);
            registers[33] = (byte)src;
        }
        registers[18] = (byte)(dest >> 8);
        registers[19] = (byte)dest;
    }
    else if (register_addr == 24)
    {
      bool reverseScreenChange = ((registers[register_addr] & 0x40) != (value & 0x40));
      registers[register_addr] = value;
      if (reverseScreenChange)
        RedrawScreen();
    }
    else if (register_addr == 25)
    {
      bool attributesEnableChange = ((registers[register_addr] & 0x40) != (value & 0x40));
      registers[register_addr] = value;
      if (attributesEnableChange)
        RedrawScreen();
    }
    else if (register_addr == 26)
    {
      bool attributesEnabled = ((registers[25] & 0x40) != 0);
      bool colorScreenChange = (attributesEnabled && (registers[register_addr] & 0xF) != (value & 0xF))
        || (!attributesEnabled && registers[register_addr] != value);
      registers[register_addr] = value;
      if (colorScreenChange)
      {
        int bg = VDCColorToLCDColor(registers[26]);
#ifdef ARDUINO_SUNTON_8048S070
        gfx->fillRect(0, 0, 800, 480, bg);
#endif    
#ifdef M5STACK
        M5.Lcd.fillRect(0, 0, 320, 240, bg);
#endif
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
        lcd.fillRect(0, 0, 320, 170, bg);
#endif
#ifdef ILI9341
        lcd.fillRect(0, 0, 240, 320, bg);
#endif
#ifdef ILI9488
        lcd.fillRect(0, 0, 320, 480, bg);
#endif
        RedrawScreen();
      }
    }
    else if (register_addr == 10)
    {
      bool cursorChanged = ((registers[register_addr] & 0x7F) != (value & 0x7F));
      registers[register_addr] = value;
      if (cursorChanged)
        DrawChar((registers[14] << 8) | registers[15]);
    }
    else if (register_addr == 15)
    {
      registers[register_addr] = value;
      byte cursorMode = (registers[10] >> 5) & 3;
      if (cursorMode != 1) // optmize out if cursor is off
        DrawChar((registers[14] << 8) | registers[15]);
    }
    else
      registers[register_addr] = value;
}

#ifdef _WINDOWS
void VDC8563::CheckPaintFrame(unsigned long micros_now)
{
    if (redrawRequiredSignal || needsPaintFrame && (micros_now - lastPaintFrame) >= paintFrameInterval)
    {
        if (redrawRequiredSignal) {
            redrawRequiredSignal = false;
            WindowsDraw::DrawBorder(0, 0, 0); // BLACK
            RedrawScreen();
        }
        WindowsDraw::EndDraw(); // will paint anything pending
        WindowsDraw::BeginDraw(); // start a new session
        lastPaintFrame = micros_now;
        needsPaintFrame = false;
    }
}
#endif // _WINDOWS
