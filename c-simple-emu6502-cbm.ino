// c-simple-emu6502-cbm.ino - Commodore Console Emulation
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
// IMPORTANT!
//
// This port is for M5Stack with IPS LCD, USB keyboard, and D64 disk (SD)
// Note: other ports are available for other hardware platforms
// Tested with Arduino 2.1.0
// Requires M5 Basic Core (or Core2, or CoreS3)
// Note: Serial diagnostics commented out so can start immediately without terminal
////////////////////////////////////////////////////////////////////////////////

#include "emuc64.h"
#include "emu6502.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

// Arduino_DataBus *bus = new Arduino_HWSPI(1/*dc*/, GFX_NOT_DEFINED/*cs*/, 7/*sclk*/, 5/*mosi*/, GFX_NOT_DEFINED/*miso*/, &SPI, true/*is_shared_interface*/);
// Arduino_GFX *gfx = new Arduino_ST7789(bus, 3/*rst*/, 1/*r*/, true/*ips*/, 240, 320);
Arduino_DataBus *bus = new Arduino_HWSPI(6/*dc*/, 7/*cs*/, 2/*sclk*/, 3/*mosi*/, 10/*miso*/, &SPI, true/*is_shared_interface*/);
Arduino_GFX *gfx = new Arduino_ST7789(bus, 11/*rst*/, 1/*r*/, true/*ips*/, 240, 320);

void setup() {
  gfx->begin();
  Serial.begin(115200);
  Serial.setTimeout(0); // so we don't wait for reads
  C64_Init();
}

void loop() {
  // put your main code here, to run repeatedly:
  ResetRun(ExecutePatch); 
}
