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

#include <SD.h>
#include <SPI.h>
#include "M5Core.h"

// globals
const char* StartupPRG = 0;
int main_go_num = 20;

void setup() {
  M5.begin();

  //Initialize serial (but don't wait for it to be connected, until there is an exception
  M5Serial.begin(115200);
  M5Serial.setTimeout(0); // so we don't wait for reads
  Serial2.begin(115200, SERIAL_8N1, SW_RX, -1);
  Serial2.setTimeout(0); // so we don't wait for reads

  if(!SD.begin(SD_CS_OVERRIDE)){
      M5Serial.println("Card Mount Failed");
      M5.Lcd.println("Card Mount Failed");
      while(1); // cannot continue, so hang around
  }
}

void loop() {
  EmuCBM* cbm;
  if (main_go_num == 128)
    cbm = new EmuC128();
  else if (main_go_num == 4)
    cbm = new EmuTed(64);
  else if (main_go_num == 16)
    cbm = new EmuTed(16);
  else if (main_go_num == 20)
    cbm = new EmuVic20(5);
  else
    cbm = new EmuC64();
  cbm->ResetRun();
  delete cbm;
  delay(1000);
}
