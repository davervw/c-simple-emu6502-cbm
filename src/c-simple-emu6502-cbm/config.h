// select options based on IDE selected board

// Serial0 is one of the serial ports used to input C64/C128 scan codes
// RX2 is a software serial line to receive C64/C128 scan codes
// scan codes are expected comma separated like 15,1 with newline at the end
// 1024=RESTORE, 128=CAPS, 256 reserved for DISP40/80 but not supported, because no 80 column support

// NOTE: M5Stack_Core_ESP32 doesn't include PSRAM so won't work with this solution (use one of other listed boards below instead).

#ifdef ARDUINO_M5STACK_Core2
#include <M5Core2.h>
#define Serial0 Serial
#define RX2 G32
#define SDA G32
#define SCL G33
#define SD_CS_OVERRIDE
#endif

#ifdef ARDUINO_M5STACK_CORES3
#include <M5Unified.h>
#define Serial0 USBSerial
#define RX2 G2
#define SDA G2
#define SCL G1
#define SD_CS_OVERRIDE GPIO_NUM_4
#endif

#ifdef ARDUINO_M5STACK_FIRE
#include <M5Unified.h>
#define Serial0 Serial
#define RX2 G21
#define SDA G21
#define SCL G22
#define SD_CS_OVERRIDE GPIO_NUM_4
#endif