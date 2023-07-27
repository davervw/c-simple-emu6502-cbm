// select board, options
#define CORES3

#ifdef BASIC_CORE
#include <M5Stack.h>
#define M5Serial Serial
#define SD_CS_OVERRIDE
#endif

#ifdef CORE2
#include <M5Core2.h>
#define M5Serial Serial
#define SD_CS_OVERRIDE
#endif

#ifdef CORES3
#include <M5Unified.h>
#define M5Serial USBSerial
#define SD_CS_OVERRIDE GPIO_NUM_4
#endif