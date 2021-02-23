// c-simple-emu6502-cbm.ino - Commodore Console Emulation
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C Portable Version)
// C64/6502 Emulator for Teensy
//
// MIT License
//
// Copyright (c) 2021 by David R. Van Wagner
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
// This port is for Teensy 4.1 with IPS LCD, USB keyboard, and D64 disk (SD)
// Note: other ports are available for other hardware platforms
// Tested with Arduino 1.8.5
// Requires Teensyduino, tested with 1.53, with USBHost_t36
// Requires Third party library:
// * https://github.com/KurtE/ILI9341_t3n.git
// Note: Serial diagnostics commented out so can start immediately without terminal
////////////////////////////////////////////////////////////////////////////////

#include "emuc64.h"
#include "emu6502.h"

#include <SD.h>
#include <SPI.h>
#include "emud64.h"

//extern "C" uint8_t external_psram_size;
//bool memory_ok = false;
//uint32_t *memory_begin = 0;
//uint32_t *memory_end = 0;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
//  while (!Serial) {
//    ; // wait for serial port to connect. Needed for native USB port only
//  }
//  Serial.println("");
//  Serial.println("c-simple-emu-cbm version 1.6 Teensy LCD/SD Edition");
//  Serial.println("Copyright (c) 2021 by David R. Van Wagner\n");
//  Serial.println("MIT License\n");
//  Serial.println("github.com/davervw\n");
//  Serial.println("\n");

  // From teensy41_psram_memtest
//  pinMode(13, OUTPUT); // ??? PSRAM ???
//  if (external_psram_size > 0)
//  {
//    const float clocks[4] = {396.0f, 720.0f, 664.62f, 528.0f};
//    const float frequency = clocks[(CCM_CBCMR >> 8) & 3] / (float)(((CCM_CBCMR >> 29) & 7) + 1);
//    //Serial.printf("PSRAM %d Mbyte\n", external_psram_size);
//    //Serial.printf(" CCM_CBCMR=%08X (%.1f MHz)\n", CCM_CBCMR, frequency);
//    memory_begin = (uint32_t *)(0x70000000);
//    memory_end = (uint32_t *)(0x70000000 + external_psram_size * 1048576);
//  }

  SD.begin(BUILTIN_SDCARD);

//  EmuD64* d64 = new EmuD64("samples.d64");
//  const char* dir_fmt = d64->GetDirectoryFormatted();
//  Serial.print(dir_fmt);
//  delete(d64);

  C64_Init();
}

void loop() {
  // put your main code here, to run repeatedly:
  ResetRun(ExecutePatch); 
}

