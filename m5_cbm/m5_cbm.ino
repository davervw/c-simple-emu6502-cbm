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
#include "emuc128.h"
#include "emuted.h"
#include "emud64.h"
#include "emuvic20.h"
#include "emupet.h"
#include "emutest.h"
#include "emu6502.h"

#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include "M5Core.h"
#include "cardkbdscan.h"

// globals
const char* StartupPRG = 0;

#ifdef TEST6502 // see emutest.cpp
int main_go_num = -1;
#else
int main_go_num = 0;
#endif

void setup() {
  M5.begin();

  //Initialize serial (but don't wait for it to be connected, until there is an exception
  M5Serial.begin(115200);
  M5Serial.setTimeout(0); // so we don't wait for reads

  if(!SD.begin(SD_CS_OVERRIDE)){
      M5Serial.println("Card Mount Failed");
      M5.Lcd.println("Card Mount Failed");
      while(1); // cannot continue, so hang around
  }

  //Serial or I2C
  Wire.begin(SDA, SCL, 100000UL);
  for (int i=1; i<=10; ++i)
  {
    if (Wire.requestFrom(0x5F, 1) == 1)
    {
      CardKbd = true;
      break;
    }
    delay(100);
  }
  if (!CardKbd)
  {
    Wire.end();
    //Initialize serial (but don't wait for it to be connected, until there is an exception
    Serial2.begin(115200, SERIAL_8N1, SW_RX, -1);
    Serial2.setTimeout(0); // so we don't wait for reads
  }

#ifdef FIRE
  pinMode(39, INPUT_PULLUP);
  pinMode(38, INPUT_PULLUP);
  pinMode(37, INPUT_PULLUP);
#endif
}

void loop() {
  Emu6502* emu;
  if (main_go_num == 128)
    emu = new EmuC128();
  else if (main_go_num == 4)
    emu = new EmuTed(64);
  else if (main_go_num == 16)
    emu = new EmuTed(16);
  else if (main_go_num == 20)
    emu = new EmuVic20(5);
  else if (main_go_num == 2001)
    emu = new EmuPET(32);
  else if (main_go_num == -1)
    emu = new EmuTest();
  else
    emu = new EmuC64();
  emu->ResetRun();
  delete emu;
  delay(1000);
}
