// LCDDraw.cpp - UI handling for embedded LCDs
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

#ifndef _WINDOWS

#include "config.h"
#include "emu6502.h"
#include "LCDDraw.h"

// native resolution (e.g. 320x240)
int LCDDraw::clientwidth;
int LCDDraw::clientheight;

// emulated system resolution (e.g. 320x200)
int LCDDraw::screenwidth;
int LCDDraw::screenheight;
int LCDDraw::borderwidth;
int LCDDraw::borderheight;

// how many native pixels to use for emulated 8 pixels
int LCDDraw::scalex;
int LCDDraw::scaley;

bool LCDDraw::CreateRenderTarget(int screenwidth, int screenheight, int scalex, int scaley)
{
#ifdef M5STACK
	clientwidth = 320;
	clientheight = 240;
#endif

	LCDDraw::screenwidth = screenwidth;
	LCDDraw::screenheight = screenheight;

	LCDDraw::scalex = scalex;
	LCDDraw::scaley = scaley;

	borderwidth = (clientwidth - (screenwidth * scalex / 8)) / 2;
	borderheight = (clientheight - (screenheight * scaley / 8)) / 2;

	if (borderwidth < 0 || borderheight < 0)
		return false;

  SerialDef.printf("screenwidth=%d screenheight=%d\n", screenwidth, screenheight);
  SerialDef.printf("scalex=%d scaley=%d\n", scalex, scaley);
  SerialDef.printf("clientwidth=%d clientheight=%d\n", clientwidth, clientheight);
  SerialDef.printf("borderwidth=%d borderheight=%d\n", borderwidth, borderheight);

	return true;
}

static int average_color(int color1, int color2)
{ // extract 565 color from 16-bit values, average them, and combine back the same way
	int red = ((color1 >> 11) + (color2 >> 11)) / 2;
	int green = (((color1 >> 5) & 0x3F) + (((color2 >> 5) & 0x3F))) / 2;
	int blue = ((color1 & 0x1F) + (color2 & 0x1F)) / 2;
	return ((red & 0x1f) << 11) | ((green & 0x3f) << 5) | (blue & 0x1F);
}

void LCDDraw::DrawCharacter2Color(byte* src, int col, int row, int fg, int bg)
{
	int x0 = borderwidth + col * scalex;
	int y0 = borderheight + row * scaley;

#ifdef M5STACK
	int colors[8];
	M5.Lcd.startWrite();
#endif
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
	int colors[4];
	int last_color;
#endif
#ifdef ILI9341
	int colors[8];
#endif
#ifdef ILI9488
	int temp = x0;
	x0 = y0;
	y0 = temp;
#define SCALEY(n) ((n*3+1)/2)
	int colors[8];
	bool fill = true;
#endif
	for (int row_i = 0; row_i < 8; ++row_i)
	{
		int mask = 128;
		bool underlined = false;
		bool isCursor = false;
		bool inverse = false;
		for (int col_i = 0; col_i < 8; ++col_i)
		{
			int color = (((src[row_i] & mask) != 0 || underlined) ^ inverse ^ isCursor) ? fg : bg;
#ifdef ARDUINO_SUNTON_8048S070      
			gfx->drawPixel(x0 + col_i, y0 + row_i * 2, color);
			gfx->drawPixel(x0 + col_i, y0 + row_i * 2 + 1, color);
#endif      
#ifdef M5STACK
			// if (col_i & 1)
			// 	M5.Lcd.drawPixel(x0 + col_i / 2, y0 + row_i, average_color(colors[col_i - 1], color));
			// colors[col_i] = color;
			M5.Lcd.drawPixel(x0 + col_i, y0 + row_i, color);
#endif
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
			if (col_i & 1)
			{
				int mid = average_color(last_color, color);
				if (row_i > 1 && row_i < 6)
					lcd.drawPixel(x0 + col_i / 2, y0 + row_i - 1, mid);
				else if (row_i == 1 || row_i == 7)
				{
					int adj = (row_i == 7) ? 2 : 1;
					mid = average_color(mid, colors[col_i / 2]);
					lcd.drawPixel(x0 + col_i / 2, y0 + row_i - adj, mid);
				}
				else
					colors[col_i / 2] = mid;
			}
			else
				last_color = color;
#endif
#ifdef ILI9341
			if (col_i & 1)
				lcd.drawPixel(y0 + row_i, 319 - (x0 + col_i / 2), average_color(colors[col_i - 1], color));
			colors[col_i] = color;
#endif
#ifdef ILI9488
			if (col_i >= 2 && col_i <= 5)
			{
				if (fill && (row_i & 1) == 1)
					lcd.drawPixel(x0 + SCALEY(row_i) - 1, 479 - (y0 + col_i - 1), average_color(color, colors[col_i]));
				lcd.drawPixel(x0 + SCALEY(row_i), 479 - (y0 + col_i - 1), color);
			}
			else if (col_i == 1 || col_i == 7)
			{
				int adj = (col_i == 7) ? 1 : 0;
				int mid = average_color(color, colors[col_i - 1]);
				if (fill && (row_i & 1) == 1)
				{
					int midabove = average_color(colors[col_i], (((src[row_i - 1] & (mask << 1)) != 0 || underlined) ^ inverse ^ isCursor) ? fg : bg);
					lcd.drawPixel(x0 + SCALEY(row_i) - 1, 479 - (y0 + col_i - 1 - adj), average_color(mid, midabove));
				}
				lcd.drawPixel(x0 + SCALEY(row_i), 479 - (y0 + col_i - 1 - adj), mid);
			}
			colors[col_i] = color;
#endif
			mask >>= 1;
		}
	}
#ifdef M5STACK   
	M5.Lcd.endWrite();
#endif
}

void LCDDraw::ClearAll(int color)
{
#ifdef M5STACK
	M5.Lcd.fillScreen(color);
#endif
#ifdef ARDUINO_SUNTON_8048S070
	gfx->fillScreen(color);
#endif
#ifdef ILI9341
	lcd.fillScreen(color);
#endif    
#ifdef ILI9488    
	lcd.fillScreen(color);
#endif   
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
	lcd.fillScreen(color);
#endif
}

void LCDDraw::ClearScreenArea(int color)
{
#ifdef M5STACK
	M5.Lcd.fillRect(borderwidth, borderheight, screenwidth * scalex / 8, screenheight * scaley / 8, color);
#endif
#ifdef ARDUINO_SUNTON_8048S070
	gfx->fillRect(borderwidth, borderheight, screenwidth * scalex / 8, screenheight * scaley / 8, color);
#endif
#ifdef ILI9341
	lcd.fillRect(borderheight, borderwidth, screenheight * scaley / 8, screenwidth * scalex / 8, color);
#endif    
#ifdef ILI9488    
	lcd.fillRect(borderheight, borderwidth, screenheight * scaley / 8, screenwidth * scalex / 8, color);
#endif   
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
	lcd.fillRect(borderwidth, borderheight, screenwidth * scalex / 8, screenheight * scaley / 8, color);
#endif
}

void LCDDraw::DrawBorder(int color)
{
#ifdef M5STACK
	M5.Lcd.startWrite();
#endif
	if (borderheight > 0) {
#ifdef M5STACK
		M5.Lcd.fillRect(0, 0, clientwidth, borderheight, color);
		M5.Lcd.fillRect(0, clientheight - borderheight, clientwidth, borderheight, color);
#endif
#ifdef ARDUINO_SUNTON_8048S070
		gfx->fillRect(borderwidth, borderheight, screenwidth * scalex / 8, screenheight * scaley / 8, color);
#endif
#ifdef ILI9341
		lcd.fillRect(0, 0, borderheight, clientwidth, color);
		lcd.fillRect(clientheight - borderheight, 0, borderheight, clientwidth, color);
#endif    
#ifdef ILI9488    
		lcd.fillRect(0, 0, borderheight, clientwidthcolor);
		lcd.fillRect(clientheight - borderheight, 0, borderheight, clientwidth, color);
#endif   
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
		lcd.fillRect(0, 0, clientwidth, borderheight, color);
		lcd.fillRect(0, clientheight - borderheight, clientwidth, borderheight, color);
#endif
	}
	if (borderwidth > 0) {
#ifdef M5STACK
		M5.Lcd.fillRect(0, borderheight, borderwidth, clientheight - borderheight * 2, color);
		M5.Lcd.fillRect(clientwidth - borderwidth, borderheight, borderwidth, clientheight - borderheight * 2, color);
#endif
#ifdef ARDUINO_SUNTON_8048S070
		gfx->fillRect(0, borderheight, borderwidth, clientheight - borderheight * 2, color);
		gfx->fillRect(clientwidth - borderwidth, borderheight, borderwidth, clientheight - borderheight * 2, color);
#endif
#ifdef ILI9341
		lcd.fillRect(0, 0, borderheight, clientwidth, color);
		lcd.fillRect(clientheight - borderheight, 0, borderheight, clientwidth, color);
#endif    
#ifdef ILI9488    
		lcd.fillRect(0, 0, borderheight, clientwidthcolor);
		lcd.fillRect(clientheight - borderheight, 0, borderheight, clientwidth, color);
#endif   
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
		lcd.fillRect(0, borderheight, borderwidth, clientheight - borderheight * 2, color);
		lcd.fillRect(clientwidth - borderwidth, borderheight, borderwidth, clientheight - borderheight * 2, color);
#endif
	}
#ifdef M5STACK   
	M5.Lcd.endWrite();
#endif
}

#endif // NOT _WINDOWS
