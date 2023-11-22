// select board, options
#define FIRE

// M5Serial is one of the serial ports used to input C64/C128 scan codes
// SW_RX is a software serial line to receive C64/C128 scan codes
// scan codes are expected comma separated like 15,1 with newline at the end
// 1024=RESTORE, 128=CAPS, 256 reserved for DISP40/80 but not supported, because no 80 column support

// NOTE: basic BASIC won't work without extra memory (use FIRE instead).  But CORES3 is somewhat faster.

#ifdef CORE2
#include <M5Core2.h>
#define M5Serial Serial
#define SW_RX G32
#define SDA G32
#define SCL G33
#define SD_CS_OVERRIDE
#endif

#ifdef CORES3
#include <M5Unified.h>
#define M5Serial USBSerial
#define SW_RX G2
#define SDA G2
#define SCL G1
#define SD_CS_OVERRIDE GPIO_NUM_4
#endif

#ifdef FIRE
#include <M5Unified.h>
#define M5Serial Serial
#define SW_RX G21
#define SDA G21
#define SCL G22
#define SD_CS_OVERRIDE GPIO_NUM_4
#endif