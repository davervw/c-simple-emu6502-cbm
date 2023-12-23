#include <Arduino.h>
#include "USBHost_t36.h"

class USBtoCBMkeyboard
{
public:
  USBtoCBMkeyboard();
  String Read();
};
