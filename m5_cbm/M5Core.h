// select board, options
#define CORES3

#ifdef BASIC_CORE
#include <M5Stack.h>
#define M5Serial Serial
#define SW_RX G21
#define SD_CS_OVERRIDE
#endif

#ifdef CORE2
#include <M5Core2.h>
#define M5Serial Serial
#define SW_RX G32
#define SD_CS_OVERRIDE
#endif

#ifdef CORES3
#include <M5Unified.h>
#define M5Serial USBSerial
#define SW_RX G2
#define SD_CS_OVERRIDE GPIO_NUM_4
#endif