// select board, options
#define CORES3

#ifdef BASIC_CORE
#include <M5Stack.h>
#define M5Serial Serial
#endif

#ifdef CORE2
#include <M5Core2.h>
#define M5Serial Serial
#endif

#ifdef CORES3
#include <M5CoreS3.h>
#define M5Serial USBSerial
#endif