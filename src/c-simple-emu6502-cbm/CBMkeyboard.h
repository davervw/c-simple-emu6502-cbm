#pragma once

class CBMkeyboard {
public:
  typedef enum { C64, C128, VIC20 } Model;  
  static int scan_codes[16]; // array allows multiple keys/modifiers pressed at one time
  static void waitKeysReleased(Model model);
  static void ReadKeyboard(Model model);
  static void reset(Model model);
  static bool caps;
};
