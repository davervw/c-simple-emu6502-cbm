// HIDtoCBMkeyboard.cpp - HID to Commodore keyboard driver
//
////////////////////////////////////////////////////////////////////////////////
//
// MIT License
//
// Copyright (c) 2026 by David R. Van Wagner
// davevw.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include "HIDtoCBMkeyboard.h"
#include "C128ScanCode.h"

// array allows multiple keys/modifiers pressed at one time
static int scan_codes[9] = { 88, 88, 88, 88, 88, 88, 88, 88, 88 } ;

static const int LIMITKEYS = 110; // include F18 but not F19

// HID key code to C64 keyboard scan code, plus shift modifiers
// +1024 means to apply RESTORE (SCAN_CODE_FLAG_RESTORE)
// +2048 means to apply L.Shift (SCAN_CODE_FLAG_FORCE_SHIFT)
// +4096 means to take away Shift (SCAN_CODE_FLAG_FORCE_NOSHIFT)
// +8192 means to apply Commodore (SCAN_CODE_FLAG_FORCE_COMMODORE)
static int usb_to_c64[2][LIMITKEYS] = {
{ // normal/other modifier
  88, 88, 88, 88, 10, 28, 20, 18, 14, 21, // na, na, na, na, a, b, c, d, e, f
  26, 29, 33, 34, 37, 42, 36, 39, 38, 41, // g, h, i, j, k, l, m, n, o, p,
  62, 17, 13, 22, 30, 31, 9, 23, 25, 12, // q, r, s, t, u, v, w, x, y, z
  56, 59, 8, 11, 16, 19, 24, 27, 32, 35, // 1, 2, 3, 4, 5, 6, 7, 8, 9, 0
  1, 63, 0, 58, 60, 43, 53, 2048+45, 2048+50, 48, // RET, STOP, DEL, CTRL, SPC, -, =, [, ], £
  88, 50, 2048+24, 8192+17, 47, 44, 55, 88, 4, 2048+4, // na, ;, ', `, ,, ., /, na, f1, f2
  5, 2048+5, 6, 2048+6, 3, 2048+3, 88, 88, 88, 88, // f3, f4, f5, f6, f7, f8, na, na, na, na
  1024, 88, 63, 2048+0, 51, 1024, 0, 88, 88, 2, // RESTORE, na, STOP, INS, HM, RESTORE, DEL, na, na, RT
  2048+2, 7, 2048+7, 88, 55, 49, 43, 40, 1, 56, // LT, DN, UP, na, /, *, -, +, ENTER, 1
  59, 8, 11, 16, 19, 24, 27, 32, 35, 44, // 2, 3, 4, 5, 6, 7, 8, 9, 0, . (keypad)
  88, 88, 88, 53, 88, 88, 88, 88, 88, 88 // na, na, na, = (keypad), na, na, na, na, na, na
},
{ // shift modifier
  88, 88, 88, 88, 10, 28, 20, 18, 14, 21, // na, na, na, na, a, b, c, d, e, f
  26, 29, 33, 34, 37, 42, 36, 39, 38, 41, // g, h, i, j, k, l, m, n, o, p,
  62, 17, 13, 22, 30, 31, 9, 23, 25, 12, // q, r, s, t, u, v, w, x, y, z
  56, 4096+46, 8, 11, 16, 4096+54, 19, 4096+49, 27, 32, // !, @, #, $, %, ^, &, *, (, )
  1, 63, 0, 58, 60, 4096+57, 4096+40, 4096+8192+62, 4096+8192+9, 4096+8192+43, // RET, STOP, DEL, CTRL, SPC, L.Arrow, +, {, }, |
  88, 4096+45, 2048+59, 8192+14, 47, 44, 55, 88, 4, 2048+4, // na, :, ", ~, ,, ., /, na, f1, f2
  5, 2048+5, 6, 2048+6, 3, 2048+3, 88, 88, 88, 88, // f3, f4, f5, f6, f7, f8, na, na, na, na
  1024, 88, 63, 2048+0, 51, 1024, 0, 88, 88, 2, // RESTORE, na, STOP, INS, HM, RESTORE, DEL, na, na, RT
  2048+2, 7, 2048+7, 88, 55, 49, 43, 40, 1, 56, // LT, DN, UP, na, /, *, -, +, ENTER, 1
  59, 8, 11, 16, 19, 24, 27, 32, 35, 44, // 2, 3, 4, 5, 6, 7, 8, 9, 0, . (keypad)
  88, 88, 88, 4096+53, 88, 88, 88, 88, 88, 88 // na, na, na, = (keypad), na, na, na, na, na, na
}
};

// Resources
// See Keyboard/Keypad Page (0x07) of https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf
// See Commodore 64 Keycodes in Appendix of https://archive.org/details/Compute_s_Mapping_the_64_and_64C/

// example PC (US) keyboard layout
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
void HIDtoCBMkeyboard::OnKeyData(uint8_t len, uint8_t* data)
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
      scan_codes[6] = SCAN_CODE_COMMODORE; // usb hid buffer supports 6 simultaneous keys, put modifier in next available slot
    else
      scan_codes[6] = 88;
      
    if ((data[0] & 2) != 0) // LShift
      scan_codes[7] = SCAN_CODE_LSHIFT; // usb hid buffer supports 6 simultaneous keys, put modifier in next available slot
    else
      scan_codes[7] = 88;
      
    if ((data[0] & 0x20) != 0) // RShift
      scan_codes[8] = SCAN_CODE_RSHIFT; // usb hid buffer supports 6 simultaneous keys, put modifier in next available slot
    else
      scan_codes[8] = 88;

    for (int i=0; i<6; ++i)
    {
      if (data[i+2] < LIMITKEYS)
      {
        scan_codes[i] = usb_to_c64[((data[0] & 0x22) != 0) ? 1 : 0][data[i+2]]; // Normal vs. Shift
        if ((scan_codes[i] & SCAN_CODE_FLAG_FORCE_SHIFT) != 0)
          scan_codes[7] = SCAN_CODE_LSHIFT;
        if (i==0 && (scan_codes[i] & SCAN_CODE_FLAG_FORCE_NOSHIFT) != 0) // remove shift flag works only if key is first non-modifier pressed
        {
          scan_codes[7] = 88; // No LShift
          scan_codes[8] = 88; // No RShift
        }
        if ((scan_codes[i] & SCAN_CODE_FLAG_FORCE_COMMODORE) != 0)
          scan_codes[7] = SCAN_CODE_COMMODORE;
        // if (scan_codes[i] != 88)
        // {
        //    SerialDef.printf("[%d] %d ", i, scan_codes[i]);
        // }
      }
      else
        scan_codes[i] = 88;
    }
  }
}

String HIDtoCBMkeyboard::Read()
{
  static String lastkeys = "";
  String s = "";
  for (int i = 0; i < 9; ++i)
  {
    scan_codes[i] &= ~(SCAN_CODE_FLAG_FORCE_SHIFT | SCAN_CODE_FLAG_FORCE_NOSHIFT | SCAN_CODE_FLAG_FORCE_COMMODORE);
    if (scan_codes[i] == 88)
      continue;
    char buffer[8];
    if (s.length() > 0)
      s = s + ',';
    itoa(scan_codes[i], buffer, 10);
    s = s + buffer;
  }
  if (s.length() > 0)
  { // keys pressed
    s = s + '\n';
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
  s = "88\n";
  //SerialDef.print(s);
  return s;
}
