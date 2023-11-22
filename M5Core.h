// select board, options
#define BASIC_CORE

#ifdef BASIC_CORE
#include <M5Unified.h> // tested with M5Stack 2.0.6
#define M5Serial Serial
#define SDA G22
#define SCL G21
#endif

#ifdef CORE2
#include <M5Core2.h>
#define M5Serial Serial
#endif

#ifdef CORES3
#include <M5CoreS3.h>
#define M5Serial USBSerial
#endif