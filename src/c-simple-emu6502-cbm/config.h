// select options based on IDE selected board

// SerialDef is one of the serial ports used to input C64/C128 scan codes
// RX2 is a software serial line to receive C64/C128 scan codes
// scan codes are expected comma separated like 15,1 with newline at the end
// 1024=RESTORE, 128=CAPS, 256 reserved for DISP40/80 but not supported, because no 80 column support

// NOTE: M5Stack_Core_ESP32 doesn't include PSRAM so won't work with this solution (use one of other listed boards below instead).

// TODO: more build instructions needed for Arduino IDE options because they can get tricky

////////////////////////////
#ifdef ARDUINO_M5STACK_Core2
#include <M5Core2.h> // library version 0.1.9
#define SerialDef Serial
#define RX2 G32
#define SDA G32
#define SCL G33
#define SD_CS_OVERRIDE
#define M5STACK
#endif
////////////////////////////
#ifdef ARDUINO_M5STACK_CORES3
#include <M5Unified.h> // library version 0.1.14
#define SerialDef USBSerial
#define RX2 G2
#define SDA G2
#define SCL G1
#define SD_CS_OVERRIDE GPIO_NUM_4
#define M5STACK
#endif
////////////////////////////
#ifdef ARDUINO_M5STACK_FIRE
#include <M5Unified.h> // library version 0.1.14
#define SerialDef Serial
#define RX2 G21
#define SDA G21
#define SCL G22
#define SD_CS_OVERRIDE GPIO_NUM_4
#define M5STACK
#endif
////////////////////////////
// In case Sunton board is not defined, use selection of ESP32S3 Dev Module instead
#ifdef ARDUINO_ESP32S3_DEV
#define ARDUINO_SUNTON_8048S070
#endif
#ifdef ARDUINO_SUNTON_8048S070
#include <Arduino.h>
#include <Arduino_GFX_Library.h> // GFX Library for Arduino library version 1.3.1
#include <cstring>
#define SerialDef Serial
#define RX2 -1
#define SDA -1
#define SCL -1
extern Arduino_RPi_DPI_RGBPanel *gfx;
#endif
////////////////////////////
#ifdef ARDUINO_TEENSY41
#define RX2 -1
#define SerialDef Serial
//#define ILI9341
#define ILI9488
#include <SPI.h>
#ifdef ILI9341
#include "ILI9341_t3n.h" // library version 1.1.1
extern ILI9341_t3n lcd;
#endif
#ifdef ILI9488
#include "ILI9488_t3.h" // included with Teensy Board Support
extern ILI9488_t3 lcd;
#endif
#endif
////////////////////////////
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
#define RX2 -1
#define SerialDef Serial
#include <Arduino.h>
#include <TFT_eSPI.h> // library version 2.5.0
extern TFT_eSPI lcd;
#endif
