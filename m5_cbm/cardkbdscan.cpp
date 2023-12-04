// cardkbdscan.cpp
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C Portable Version)
// C64/6502 Emulator for M5Stack Cores
//
// MIT License
//
// Copyright (c) 2023 by David R. Van Wagner
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

#include "M5Core.h"
#include "cardkbdscan.h"
#include <Wire.h>

static int xlat[256] = {
  64, 64, 64, 64, 64, 64, 64, 64, 0, 128+63, 64, 64, 64, 1, 64, 64, // 0x00-0x0f
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 63, 64, 64, 64, 64, // 0x10-0x1f
  60, 128+56, 128+59, 128+8, 128+11, 128+16, 128+19, 128+24, 128+27, 128+32, 49, 40, 47, 43, 44, 55, // 0x20-0x2f
  35, 56, 59, 8, 11, 16, 19, 24, 27, 32, 45, 50, 128+47, 53, 128+44, 128+55, // 0x30-0x3f
  46, 128+10, 128+28, 128+20, 128+18, 128+14, 128+21, 128+26, 128+29, 128+33, 128+34, 128+37, 128+42, 128+36, 128+39, 128+38, // 0x40-0x4f
  128+41, 128+62, 128+17, 128+13, 128+22, 128+30, 128+31, 128+9, 128+23, 128+25, 128+12, 128+45, 48, 128+50, 54, 57, // 0x50-0x5f
  64, 10, 28, 20, 18, 14, 21, 26, 29, 33, 34, 37, 42, 36, 39, 38, // 0x60-0x6f
  41, 62, 17, 13, 22, 30, 31, 9, 23, 25, 12, 64, 64, 64, 128+54, 128+0, // 0x70-0x7f
  1024+63, 512+56, 512+59, 512+8, 512+11, 512+16, 512+19, 512+24, 512+27, 512+32, 512+35, 128+0, 1024+64, 256+62, 256+9, 256+14, // 0x80-0x8f
  256+17, 256+22, 256+25, 256+30, 256+33, 256+38, 256+41, 64, 51, 128+51, 256+10, 256+13, 256+18, 256+21, 256+26, 256+29, // 0x90-0x9f
  256+34, 256+37, 256+42, 128+1, 128+256+64, 128+49, 256+12, 256+23, 256+20, 256+31, 256+28, 256+39, 256+36, 256+46, 128+46, 128+60, // 0xa0-0xaf
  64, 64, 64, 64, 128+2, 128+7, 7, 2, 64, 64, 64, 64, 64, 64, 64, 64, // 0xb0-0xbf
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // 0xc0-0xcf
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // 0xd0-0xdf
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // 0xe0-0xef
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // 0xf0-0xff
};

static char keys[80];

bool CardKbd = false;

// M5Stack CardKB support
// tested with Unit V1.1
// https://shop.m5stack.com/products/cardkb-mini-keyboard-programmable-unit-v1-1-mega8a

String CardKbdScanRead()
{
  String s = "";
  static unsigned long timeout = 1000000 / 20; // multiple of 1/60 of second
  static unsigned long timer_then = micros();

  if ((micros()-timer_then) < timeout)
    return s;

  timer_then = micros();

  // put your main code here, to run repeatedly:
  Wire.requestFrom(0x5F, 1);

  while (Wire.available()) {
    uint8_t data = Wire.read();
    if (data != 0)
    {
      int scan = xlat[data];
      bool shift = (scan & 128) != 0;
      bool cbm = (scan & 256) != 0;
      bool ctrl = (scan & 512) != 0;
      bool restore = (scan & 1024) != 0;
      scan &= 127;
      char* dest = keys;
      *dest = 0;

      if (shift)
      {
        strcat(keys, "15,");
        dest = keys + strlen(keys);
      }
      if (cbm)
      {
        strcat(keys, "61,");
        dest = keys + strlen(keys);
      }
      if (ctrl)
      {
        strcat(keys, "58,");
        dest = keys + strlen(keys);
      }
      if (restore)
      {
        strcat(keys, "1024,");
        dest = keys + strlen(keys);
      }
      
      if (scan != 64)
        itoa(scan, dest, 10);
      if (*keys != 0)
      {
        s = keys;
        s += '\n';
        return s;
      }
    }
  }

  if (keys[0] != 0)
  {
    s = "64\n";
    keys[0] = 0;
  }

  return s;
}
