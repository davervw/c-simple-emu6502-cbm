// c-simple-emu6502-cbm.ino - Commodore Console Emulation
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C Portable Version)
// C64/6502 Emulator for Microsoft Windows Console
//
// MIT License
//
// Copyright(c) 2020 by David R.Van Wagner
// davevw.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//
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
// This port is for Arduino Due (ARM ATSAM3X8E).  Should be flexible enough to
//   port to other Arduino platforms with lots of flash and RAM.  C64 ROMs require
//   16K flash, C64 RAM can be configured from 64K down to 3K and still be usable.
// Requires almost 64K Flash, a bit more than 64K RAM
// Expects a vt100 terminal in line mode, sending carriage returns as line endings
// Usually in echo mode, but Arduino serial monitor will do as well.
// Put keyboard in CAPS LOCK to type BASIC commands in uppercase like PRINT 2+2
// Use DUE Programming Port for USB Serial connection (Serial) that is used
//   througout the various files of this solution.
////////////////////////////////////////////////////////////////////////////////

#include "emuc64.h"
#include "emu6502.h"

void setup() {
  // put your setup code here, to run once:
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("");
  Serial.println("c-simple-emu-cbm version 1.4");
  Serial.println("Copyright (c) 2020 by David R. Van Wagner\n");
  Serial.println("MIT License\n");
  Serial.println("github.com/davervw\n");
  Serial.println("\n");

  //StartupPRG = "/local/guess2.prg";
  C64_Init();
}

void loop() {
  // put your main code here, to run repeatedly:
  ResetRun(ExecutePatch); 
}

