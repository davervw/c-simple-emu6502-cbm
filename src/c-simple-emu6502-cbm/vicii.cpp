// vicii.cpp - C64 VIC-II Video Chip Emulation
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

#include "vicii.h"
#include "config.h"

#ifdef _WINDOWS
#include <Windows.h>
#include <WindowsDraw.h>
#include <WindowsTime.h>
#endif

EmuVicII::EmuVicII(byte* vram, byte* vio, byte* vcolor_nybles, byte* vchargen)
{
    ram = vram;
    io = vio;
    color_nybles = vcolor_nybles;
    chargen = vchargen;
    old_video = new byte[1000];
    old_color = new byte[1000];
    postponeDrawChar = false;
    active = true;
    border = 0;
    video_addr = 0x0400;
    chargen_addr = 0xD000;

    memset(old_video, 32, 1000);
    memset(old_color, 0, 1000);

#ifdef _WINDOWS
    if (!WindowsDraw::CreateRenderTarget(320, 200, 32, 32, redrawRequiredSignal))
        throw "CreateRenderTarget failed";
    WindowsDraw::BeginDraw();
    unsigned long last_refresh = micros();
    needsPaintFrame = false;
#endif

    // initialize LCD screen
#ifdef _WINDOWS
    WindowsDraw::ClearScreen(0, 0, 0); // BLACK
#endif
#ifdef M5STACK
    M5.Lcd.fillScreen(0x0000);  // BLACK
#endif
#ifdef ARDUINO_SUNTON_8048S070
    gfx->fillScreen(0x0000);  // BLACK
#endif
}

EmuVicII::~EmuVicII()
{
    delete[] old_video;
    delete[] old_color;
#ifdef _WINDOWS
    WindowsDraw::EndDraw();
#endif
}

void EmuVicII::Activate()
{
    if (!active)
    {
        active = true;
#ifdef _WINDOWS
        if (!WindowsDraw::CreateRenderTarget(320, 200, 32, 32, redrawRequiredSignal))
            throw "CreateRenderTarget failed";
        WindowsDraw::BeginDraw();
        unsigned long last_refresh = micros();
        needsPaintFrame = false;
#endif
        DrawBorder(border);
        RedrawScreen();
    }
}

void EmuVicII::Deactivate()
{
    active = false;
#ifdef _WINDOWS
    WindowsDraw::EndDraw();
#endif
}

void EmuVicII::UpdateAddresses()
{
    int chargen_1k_offset = io[0x18] & 0xE;
    int video_1k_offset = io[0x18] >> 4;
    int vicii_bank = ~io[0xD00] & 3;
    ushort new_video_addr = vicii_bank * 0x4000 + video_1k_offset * 0x0400;
    ushort new_chargen_addr = ((chargen_1k_offset == 4 || chargen_1k_offset == 6) ? 3 : vicii_bank) * 0x4000 + chargen_1k_offset * 0x0400;
    if (new_video_addr != video_addr || new_chargen_addr != chargen_addr) {
        video_addr = new_video_addr;
        chargen_addr = new_chargen_addr;
        RedrawScreen(); // upper to lower or lower to upper
    }
}

#ifdef _WINDOWS
void EmuVicII::CheckPaintFrame(unsigned long micros_now)
{
    if (redrawRequiredSignal || needsPaintFrame && (micros_now - lastPaintFrame) >= paintFrameInterval)
    {
        if (redrawRequiredSignal) {
            redrawRequiredSignal = false;
            DrawBorder(border);
            RedrawScreen();
        }
        WindowsDraw::EndDraw(); // will paint anything pending
        WindowsDraw::BeginDraw(); // start a new session
        lastPaintFrame = micros_now;
        needsPaintFrame = false;
    }
}
#endif // _WINDOWS

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
#ifdef _WINDOWS
    case 0: return 0x0000; // BLACK
    case 1: return 0xFFFF; // WHITE
    case 2: return 0xF800; // RED
    case 3: return 0x07FF; // CYAN
    case 4: return 0x8118; // DARKMAGENTA OR PURPLE
    case 5: return 0x0400; // DARKGREEN
    case 6: return 0x0018; // BLUE
    case 7: return 0xFFE0; // YELLOW
    case 8: return 0xFCC0; // ORANGE
    case 9: return 0x8283; // BROWN
    case 10: return 0xFC10; // PINK OR LT RED
    case 11: return 0x8410; // DARKGREY
    case 12: return 0x0208; // DARKCYAN
    case 13: return 0xBFF7; // LIGHTGREEN
    case 14: return 0x841F; // LIGHTBLUE
    case 15: return 0x4208; // LIGHTGREY
#endif
    default: return 0;
}
}

#ifdef ILI9488  
int static scale_index(int i)
{
    return (i * 3 + 1) / 2;
}
int static average_color(int color0, int color2)
{
    if (color0 == color2)
        return color0;
    unsigned char r0, g0, b0, r1, g1, b1, r2, g2, b2;
    lcd.color565toRGB(color0, r0, g0, b0);
    lcd.color565toRGB(color2, r2, g2, b2);
    r1 = (r0 + r2) / 2;
    g1 = (g0 + g2) / 2;
    b1 = (b0 + b2) / 2;
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
    r1 = (r0 + r2 + r3 + r4) / 4;
    g1 = (g0 + g2 + g3 + g4) / 4;
    b1 = (b0 + b2 + b3 + b4) / 4;
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

#ifdef _WINDOWS
static void Extract565Color(int color, byte& red, byte& green, byte& blue)
{
    red = (color >> 11) * 255 / 32;
    green = ((color >> 5) & 0x3F) * 255 / 64;
    blue = (color & 0x1F) * 255 / 32;
}
#endif

void EmuVicII::DrawChar(byte c, int col, int row, int fg, int bg)
{
    if (postponeDrawChar || !active)
        return;

    const byte* shape = (chargen_addr == 0xD000 || chargen_addr == 0xD800)
        ? &chargen[chargen_addr - 0xD000 + c * 8]
        : &ram[chargen_addr + c * 8];

#ifdef _WINDOWS
    byte fg_red, fg_green, fg_blue;
    byte bg_red, bg_green, bg_blue;
    Extract565Color(fg, fg_red, fg_green, fg_blue);
    Extract565Color(bg, bg_red, bg_green, bg_blue);
    WindowsDraw::DrawCharacter2Color(shape, col, row, fg_red, fg_green, fg_blue, bg_red, bg_green, bg_blue);
    needsPaintFrame = true;
    return;
#endif

#ifdef M5STACK  
    M5.Lcd.startWrite();
#endif  
#ifdef M5STACK  
    int x0 = 0 + col * 8;
    int y0 = 20 + row * 8;
#endif
#ifdef ARDUINO_SUNTON_8048S070
    int x0 = 80 + col * 16;
    int y0 = 40 + row * 16;
#endif
#ifdef ILI9341  
    int x0 = col * 8;
    int y0 = 20 + row * 8;
#endif  
#ifdef ILI9488  
    int x0 = 10 + row * 12;
    int y0 = col * 12;
#endif
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    int x0 = 0 + col * 8;
    int y0 = 10 + row * 6;
#endif  
    for (int row_i = 0; row_i < 8; ++row_i)
    {
        int mask = 128;
        for (int col_i = 0; col_i < 8; ++col_i)
        {
            int color = ((shape[row_i] & mask) == 0) ? bg : fg;
#ifdef M5STACK      
            M5.Lcd.drawPixel(x0 + col_i, y0 + row_i, color);
#endif
#ifdef ARDUINO_SUNTON_8048S070
            gfx->drawPixel(x0 + col_i * 2, y0 + row_i * 2, color);
            gfx->drawPixel(x0 + col_i * 2 + 1, y0 + row_i * 2, color);
            gfx->drawPixel(x0 + col_i * 2, y0 + row_i * 2 + 1, color);
            gfx->drawPixel(x0 + col_i * 2 + 1, y0 + row_i * 2 + 1, color);
#endif
#ifdef ILI9341
            lcd.drawPixel(y0 + row_i, 319 - (x0 + col_i), color);
#endif
#ifdef ILI9488
            lcd.drawPixel(x0 + scale_index(row_i), 479 - (y0 + scale_index(col_i)), color);
            if ((col_i % 2) == 0)
            {
                int color2 = ((shape[row_i] & (mask >> 1)) == 0) ? bg : fg;
                lcd.drawPixel(x0 + scale_index(row_i), 479 - (y0 + scale_index(col_i) + 1), average_color(color, color2));
            }
            if ((row_i % 2) == 0)
            {
                int color2 = ((shape[row_i + 1] & mask) == 0) ? bg : fg;
                lcd.drawPixel(x0 + scale_index(row_i) + 1, 479 - (y0 + scale_index(col_i)), average_color(color, color2));
            }
            if ((col_i % 2) == 0 && (row_i % 2) == 0)
            {
                int color2 = ((shape[row_i] & (mask >> 1)) == 0) ? bg : fg;
                int color3 = ((shape[row_i + 1] & mask) == 0) ? bg : fg;
                int color4 = ((shape[row_i + 1] & (mask >> 1)) == 0) ? bg : fg;
                lcd.drawPixel(x0 + scale_index(row_i) + 1, 479 - (y0 + scale_index(col_i) + 1), average_color(color, color2, color3, color4));
        }
#endif
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
            if (row_i == 0) // color average the top two rows of character
                lcd.drawPixel(x0 + col_i, y0 + row_i, (((shape[0] | shape[1]) & mask) == 0) ? bg : (((shape[0] & mask) == (shape[1] & mask)) ? fg : average_color(fg, bg)));
            else if (row_i == 5) // color average the bottom two rows of character
                lcd.drawPixel(x0 + col_i, y0 + row_i, (((shape[6] | shape[7]) & mask) == 0) ? bg : (((shape[6] & mask) == (shape[7] & mask)) ? fg : average_color(fg, bg)));
            else if (row_i < 5) // keep detail of four center rows of character
                lcd.drawPixel(x0 + col_i, y0 + row_i, ((shape[row_i + 1] & mask) == 0) ? bg : fg);
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
    DrawChar(ram[video_addr + offset], col, row, fg, bg);
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
            DrawChar(ram[video_addr + offset], col, row, fg, bg);
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
            byte new_char = ram[video_addr + offset];
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
    if (!active)
        return;

    border = value;
    int color = C64ColorToLCDColor(border);
#ifdef _WINDOWS
    byte red, green, blue;
    Extract565Color(color, red, green, blue);
    WindowsDraw::DrawBorder(red, green, blue);
    needsPaintFrame = true;
#endif
#ifdef M5STACK
    M5.Lcd.startWrite();
    M5.Lcd.fillRect(0, 0, 320, 20, color);
    M5.Lcd.fillRect(0, 220, 320, 20, color);
    M5.Lcd.endWrite();
#endif    
#ifdef ARDUINO_SUNTON_8048S070
    gfx->fillRect(0, 0, 800, 40, color);
    gfx->fillRect(0, 440, 800, 40, color);
    gfx->fillRect(0, 40, 80, 400, color);
    gfx->fillRect(720, 40, 80, 400, color);
#endif
#ifdef ILI9341    
    lcd.fillRect(0, 0, 20, 320, color);
    lcd.fillRect(220, 0, 20, 320, color);
#endif    
#ifdef ILI9488    
    lcd.fillRect(0, 0, 10, 480, color);
    lcd.fillRect(310, 0, 10, 480, color);
#endif   
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    lcd.fillRect(0, 0, 320, 10, color);
    lcd.fillRect(0, 160, 320, 10, color);
#endif
}

void EmuVicII::SaveOldVideoAndColor()
{
    memcpy(&old_video[0], &ram[video_addr], 1000);
    memcpy(&old_color[0], &color_nybles[0], 1000);
}
