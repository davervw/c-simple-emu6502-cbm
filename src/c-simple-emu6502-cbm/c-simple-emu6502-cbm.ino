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
// This port is for various platforms with IPS LCD, (various) keyboard, and D64 disk (SD or FFAT)
// Note: other ports are available for other hardware platforms
// Tested with Arduino 2.1.0
// Note: Serial diagnostics commented out so can start immediately without terminal
////////////////////////////////////////////////////////////////////////////////

#include "emuc64.h"
#include "emuc128.h"
// #include "emuted.h"
#include "emud64.h"
#include "emuvic20.h"
// #include "emupet.h"
#include "emutest.h"
#include "emu6502.h"

#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
#include <FFat.h>
#else
#include <SD.h>
#include <SPI.h>
#endif
#include <Wire.h>

#include "config.h"
#include "cardkbdscan.h"
#ifndef ARDUINO_TEENSY41
#include "ble_keyboard.h"
BLE_Keyboard* ble_keyboard;
#endif

// globals
const char* StartupPRG = 0;
#ifdef ARDUINO_TEENSY41
extern "C" uint8_t external_psram_size; // https://protosupplies.com/learn/prototyping-system-for-teensy-4-1-working-with-teensy-4-1-memory/
#endif

#ifdef ARDUINO_SUNTON_8048S070
#define GFX_BL DF_GFX_BL // default backlight pin, you may replace DF_GFX_BL to actual backlight pin
#define TFT_BL 2

/* More data bus class: https://github.com/moononournation/Arduino_GFX/wiki/Data-Bus-Class */
//Arduino_DataBus *bus = create_default_Arduino_DataBus();

/* More display class: https://github.com/moononournation/Arduino_GFX/wiki/Display-Class */
//Arduino_GFX *gfx = new Arduino_ILI9341(bus, DF_GFX_RST, 0 /* rotation */, false /* IPS */);

Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
    GFX_NOT_DEFINED /* CS */, GFX_NOT_DEFINED /* SCK */, GFX_NOT_DEFINED /* SDA */,
    41 /* DE */, 40 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
    14 /* R0 */, 21 /* R1 */, 47 /* R2 */, 48 /* R3 */, 45 /* R4 */,
    9 /* G0 */, 46 /* G1 */, 3 /* G2 */, 8 /* G3 */, 16 /* G4 */, 1 /* G5 */,
    15 /* B0 */, 7 /* B1 */, 6 /* B2 */, 5 /* B3 */, 4 /* B4 */
);
// option 1:
// 7寸 50PIN 800*480
Arduino_RPi_DPI_RGBPanel *gfx = new Arduino_RPi_DPI_RGBPanel(
  bus,
//  800 /* width */, 0 /* hsync_polarity */, 8/* hsync_front_porch */, 2 /* hsync_pulse_width */, 43/* hsync_back_porch */,
//  480 /* height */, 0 /* vsync_polarity */, 8 /* vsync_front_porch */, 2/* vsync_pulse_width */, 12 /* vsync_back_porch */,
//  1 /* pclk_active_neg */, 16000000 /* prefer_speed */, true /* auto_flush */);

    800 /* width */, 0 /* hsync_polarity */, 210 /* hsync_front_porch */, 30 /* hsync_pulse_width */, 16 /* hsync_back_porch */,
    480 /* height */, 0 /* vsync_polarity */, 22 /* vsync_front_porch */, 13 /* vsync_pulse_width */, 10 /* vsync_back_porch */,
    1 /* pclk_active_neg */, 16000000 /* prefer_speed */, true /* auto_flush */);
#endif

#ifdef ARDUINO_TEENSY41
#ifdef ILI9341
ILI9341_t3n lcd = ILI9341_t3n(10 /*CS*/, 9 /*DC*/);
#endif
#ifdef ILI9488
ILI9488_t3 lcd = ILI9488_t3(10 /*CS*/, 9 /*DC*/);
#endif
#endif

#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
TFT_eSPI lcd;
#endif

#ifdef TEST6502 // see emutest.cpp
int main_go_num = -1;
#else
int main_go_num = 64;
#endif

void setup() {

#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
  SPI.begin(12, 13, 11, 10);
#endif

#ifdef M5STACK
  M5.begin();
#endif
#ifdef ARDUINO_SUNTON_8048S070
  gfx->begin();
  gfx->fillScreen(BLACK);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
#endif
#ifdef ARDUINO_TEENSY41
  lcd.begin();
  lcd.fillScreen(0x0000); // BLACK
#endif
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
  lcd.init();
  lcd.setRotation(1);
  lcd.fillScreen(TFT_BLACK);
#endif 

  //Initialize serial (but don't wait for it to be connected, until there is an exception
  SerialDef.begin(115200);
  SerialDef.setTimeout(0); // so we don't wait for reads

#ifdef SD_CS_OVERRIDE
  if(!SD.begin(SD_CS_OVERRIDE)){
#else 
 #ifdef ARDUINO_TEENSY41
  if (!SD.begin(BUILTIN_SDCARD)){
 #else
  #ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    if (!FFat.begin()) {
      Serial.println("WARNING: did not mount FFAT");
      lcd.println("FFAT Mount Failed");
  #else
  if(!SD.begin()){
  #endif
 #endif
#endif

#ifdef M5STACK
      SerialDef.println("Card Mount Failed");
      M5.Lcd.println("Card Mount Failed");
#endif
#ifdef ARDUINO_SUNTON_8048S070
      SerialDef.println("Card Mount Failed");
      gfx->println("Card Mount Failed");
#endif
#ifdef ARDUINO_TEENSY41
      SerialDef.println("Card Mount Failed");
      lcd.println("Card Mount Failed");
#endif 
      while(1); // cannot continue, so hang around
  }

  //Serial or I2C
#ifdef ARDUINO_TEENSY41
  Wire.begin();
#else
  Wire.begin(SDA, SCL, 100000UL);
#endif  
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
#ifndef ARDUINO_SUNTON_8048S070
#ifndef ARDUINO_TEENSY41
#ifndef ARDUINO_LILYGO_T_DISPLAY_S3
    //Initialize serial (but don't wait for it to be connected, until there is an exception
    Serial2.begin(115200, SERIAL_8N1, RX2, -1);
    Serial2.setTimeout(0); // so we don't wait for reads
#endif
#endif
#endif
  }

#ifdef ARDUINO_M5STACK_FIRE
  pinMode(39, INPUT_PULLUP);
  pinMode(38, INPUT_PULLUP);
  pinMode(37, INPUT_PULLUP);
#endif

#ifndef ARDUINO_TEENSY41
  ble_keyboard = new BLE_Keyboard();
#endif

#ifdef ARDUINO_TEENSY41
  SerialDef.printf("PSRAM Memory Size = %d Mbyte\n", external_psram_size);
#endif
}

void loop() {
  Emu6502* emu;
  if (main_go_num == 128)
    emu = new EmuC128();
  // else if (main_go_num == 4)
  //   emu = new EmuTed(64);
  // else if (main_go_num == 16)
  //   emu = new EmuTed(16);
  else if (main_go_num == 20)
    emu = new EmuVic20(5);
  // else if (main_go_num == 2001)
  //   emu = new EmuPET(32);
  else if (main_go_num == -1)
    emu = new EmuTest();
  else
    emu = new EmuC64();
  emu->ResetRun();
  delete emu;
  delay(1000);
}
