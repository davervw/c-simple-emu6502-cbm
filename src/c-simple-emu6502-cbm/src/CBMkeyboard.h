#pragma once

class CBMkeyboard {
public:
  typedef enum { C64, C128, VIC20 } Model;  
  static int scan_codes[16]; // array allows multiple keys/modifiers pressed at one time
  static void waitKeysReleased(Model model);
  static void ReadKeyboard(Model model);
  static void reset(Model model);
  static bool caps;
  static bool heldToggle;
  static uint8_t joystick_c64_1; // active low bits 0:up, 1:down, 2:left, 3:right, 4:fire
  static uint8_t joystick_c64_2; // active low bits 0:up, 1:down, 2:left, 3:right, 4:fire
  static uint8_t joystick_vic20_1; // active low bits 1:right, 3:up, 4:left, 5:fire
  static uint8_t joystick_vic20_2; // active low bits 7:down
};
