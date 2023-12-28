#include "config.h"
#ifdef ARDUINO_TEENSY41
#include "USBtoCBMkeyboard.h"

static USBHost myusb;
static USBHub hub1(myusb);
static USBHub hub2(myusb);
static KeyboardController keyboard1(myusb);
static USBDriver *drivers[] = {&hub1, &hub2, &keyboard1};

// array allows multiple keys/modifiers pressed at one time
static int scan_codes[9] = { 64, 64, 64, 64, 64, 64, 64, 64, 64 } ;

// USB key code to C64 keyboard scan code, plus shift modifiers
// +256 means to apply L.Shift
// -512 means to take away Shift
// +1024 means to apply RESTORE
static int usb_to_c64[2][100] = {
{ // normal/other modifier
  64, 64, 64, 64, 10, 28, 20, 18, 14, 21, // na, na, na, na, a, b, c, d, e, f
  26, 29, 33, 34, 37, 42, 36, 39, 38, 41, // g, h, i, j, k, l, m, n, o, p,
  62, 17, 13, 22, 30, 31, 9, 23, 25, 12, // q, r, s, t, u, v, w, x, y, z
  56, 59, 8, 11, 16, 19, 24, 27, 32, 35, // 1, 2, 3, 4, 5, 6, 7, 8, 9, 0
  1, 63, 0, 58, 60, 43, 53, 256+45, 256+50, 48, // RET, STOP, DEL, CTRL, SPC, -, =, [, ], £
  64, 50, 256+24, 64, 47, 44, 55, 64, 4, 256+4, // na, ;, ', na, ,, ., /, na, f1, f2
  5, 256+5, 6, 256+6, 3, 256+3, 64, 64, 64, 64, // f3, f4, f5, f6, f7, f8, na, na, na, na
  1024, 64, 63, 256+0, 51, 1024, 0, 64, 64, 2, // RESTORE, na, STOP, INS, HM, RESTORE, DEL, na, na, RT
  256+2, 7, 256+7, 64, 55, 49, 43, 40, 1, 56, // LT, DN, UP, na, /, *, -, +, ENTER, 1
  59, 8, 11, 16, 19, 24, 27, 32, 35, 44 // 2, 3, 4, 5, 6, 7, 8, 9, 0, . (keypad)
},
{ // shift modifier
  64, 64, 64, 64, 10, 28, 20, 18, 14, 21, // na, na, na, na, a, b, c, d, e, f
  26, 29, 33, 34, 37, 42, 36, 39, 38, 41, // g, h, i, j, k, l, m, n, o, p,
  62, 17, 13, 22, 30, 31, 9, 23, 25, 12, // q, r, s, t, u, v, w, x, y, z
  56, 512+46, 8, 11, 16, 512+54, 19, 512+49, 27, 32, // !, @, #, $, %, ^, &, *, (, )
  1, 63, 0, 58, 60, 512+57, 512+40, 256+45, 256+50, 48, // RET, STOP, DEL, CTRL, SPC, L.Arrow, +, [, ], £
  64, 512+45, 256+59, 64, 47, 44, 55, 64, 4, 256+4, // na, :, ", na, ,, ., /, na, f1, f2
  5, 256+5, 6, 256+6, 3, 256+3, 64, 64, 64, 64, // f3, f4, f5, f6, f7, f8, na, na, na, na
  1024, 64, 63, 256+0, 51, 1024, 0, 64, 64, 2, // RESTORE, na, STOP, INS, HM, RESTORE, DEL, na, na, RT
  256+2, 7, 256+7, 64, 55, 49, 43, 40, 1, 56, // LT, DN, UP, na, /, *, -, +, ENTER, 1
  59, 8, 11, 16, 19, 24, 27, 32, 35, 44 // 2, 3, 4, 5, 6, 7, 8, 9, 0, . (keypad)
}
};

// Resources
// See Keyboard/Keypad Page (0x07) of https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf
// See Commodore 64 Keycodes in Appendix of https://archive.org/details/Compute_s_Mapping_the_64_and_64C/

// The initial goal of the keyboard tables and logic is to provide access to at 
// least the following keys from a standard USB keyboard, symbolic mapping more 
// like a PC (not positional)
// USB keyboard support is provided by USBHOST library and STM32F429 BSP
//
// STOP(ESC) F1 F2 F3 F4 F5 F6 F7 F8            Restore(PrtScr/SysRq) Run/Stop(Pause/Break)
//           1! 2@ 3# 4$ 5% 6^ 7& 8* 9( 0) -_ += Del/Ins(Back)    Ins Hme/Clr     / * -
// Ctrl(Tab) Q  W  E  R  T  Y  U  I  O  P  [  ]  £ (\)            Del           7 8 9 +
//           A  S  D  F  G  H  J  K  L  ;: '" Return(ENTER)                     4 5 6
// LShift    Z  X  C  V  B  N  M  ,< .> /?  RShift                     Up       1 2 3
// C=(Ctrl)           SPACEBAR              C=(Ctrl)              Lft Down Rt   0 .   Enter
//
// Note C64 Ctrl key is the PC Tab key
// Note C64 Commodore key is the PC Ctrl key
// Note keys and modifiers not shown do nothing, not support for C64
// Note PgUp is also Restore key
//
// Also, most Commodore key combinations should work for alphanumeric, some will 
// be different/missing for punctuation, full mapping of PETSCII will require 
// additional development

// modifier at index zero, keys at indexes 2..7
static uint8_t kbd_data[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
enum kbd_mod { lctrl=0x01, rctrl=0x10, lshift=0x02, rshift=0x20 }; // only tracking ones we care about

// this method expects raw keyboard HID data buffer, specifically 8 length data
static void onKeyData(uint8_t len, uint8_t* data)
{
//  static DigitalOut led2(LED2);
//  led2 = !led2;
//  
//  Serial.printf("len=%d", len);
//  for (int i=0; i<len; ++i)
//     Serial.printf(" %02X", data[i]);
//  Serial.printf("\n");
  
  if (len == 8)
  { 
    if ((data[0] & 0x11) != 0) // PC Ctrl => Commodore
      scan_codes[6] = 61; // usb hid buffer supports 6 simultaneous keys, put modifier in next available slot
    else
      scan_codes[6] = 64;
      
    if ((data[0] & 2) != 0) // LShift
      scan_codes[7] = 15; // usb hid buffer supports 6 simultaneous keys, put modifier in next available slot
    else
      scan_codes[7] = 64;
      
    if ((data[0] & 0x20) != 0) // RShift
      scan_codes[8] = 52; // usb hid buffer supports 6 simultaneous keys, put modifier in next available slot
    else
      scan_codes[8] = 64;

      for (int i=0; i<6; ++i)
      {
      if (data[i+2] < 100)
      {
        scan_codes[i] = usb_to_c64[((data[0] & 0x22) != 0) ? 1 : 0][data[i+2]]; // Normal vs. Shift
        if ((scan_codes[i] & 256) != 0)
          scan_codes[7] = 15; // LShift
        if (i==0 && (scan_codes[i] & 512) != 0) // remove shift flag works only if key is first non-modifier pressed
        {
          scan_codes[7] = 64; // No LShift
          scan_codes[8] = 64; // No RShift
        }
        // if (scan_codes[i] != 64)
        // {
        //    SerialDef.printf("[%d] %d ", i, scan_codes[i]);
        // }
      }
      else
        scan_codes[i] = 64;
    }
  }
}

// original code expected keyboard HID buffer, so reconstruct it from events we receive from our keyboard handler
// place key into kbd_data indexes 2..7, or update modifier at index 0
static void onKbdRawPress(uint8_t key)
{
  if (key == 0x68)
    kbd_data[0] |= lshift;
  else if (key == 0x6c)
    kbd_data[0] |= rshift;
  else if (key == 0x67)
    kbd_data[0] |= lctrl;
  else if (key == 0x6b)
    kbd_data[0] |= rctrl;
  else
  {
    int i=2;
    while (i < 8 && kbd_data[i] != 0 && kbd_data[i] != key) // find key or end/slot
      ++i;
    if (i < 8 && kbd_data[i] == 0) // not found, have slot
      kbd_data[i] = key; // store key
  }
  onKeyData(8, &kbd_data[0]);
}

// original code expected keyboard HID buffer, so reconstruct it from events we receive from our keyboard handler
// remove key from kbd_data indexes 2..7, or update modifier at index 0
static void onKbdRawRelease(uint8_t key)
{
  if (key == 0x68)
    kbd_data[0] &= ~lshift;
  else if (key == 0x6c)
    kbd_data[0] &= ~rshift;
  else if (key == 0x67)
    kbd_data[0] &= ~lctrl;
  else if (key == 0x6b)
    kbd_data[0] &= ~rctrl;
  else
  {
    int i=2;
    while (i < 8 && kbd_data[i] != 0 && kbd_data[i] != key) // find key or end/slot
      ++i;
    if (i < 8 && kbd_data[i] == key) // found
    {
      while (i++ < 8)
      {
        if (i < 8)
          kbd_data[i-1] = kbd_data[i];
        else
          kbd_data[i-1] = 0;
      }
    }
  }
  onKeyData(8, &kbd_data[0]);
}

USBtoCBMkeyboard::USBtoCBMkeyboard()
{
  myusb.begin();
  keyboard1.attachRawPress(onKbdRawPress);
  keyboard1.attachRawRelease(onKbdRawRelease);
}

static String lastkeys;

String USBtoCBMkeyboard::Read()
{
  String s = "";
  for (int i = 0; i < 9; ++i)
  {
    if (scan_codes[i] & 256)
      scan_codes[i]^=256;
    if (scan_codes[i] == 64)
      continue;
    char buffer[8];
    if (s.length() > 0)
      s.append(',');
    itoa(scan_codes[i], buffer, 10);
    s.append(buffer);
  }
  if (s.length() > 0)
  { // keys pressed
    s.append('\n');
    if (s == lastkeys)
      return ""; // no new information
    //SerialDef.print(s);
    lastkeys = s;
    return s;
  } 
  else if (lastkeys.length() == 0) // no key before
    return ""; // no new information

  // was key before, but released
  lastkeys = s;
  s = "64\n";
  //SerialDef.print(s);
  return s;
}
#endif