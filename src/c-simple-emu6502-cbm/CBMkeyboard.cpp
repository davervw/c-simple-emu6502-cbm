#include "config.h"
#include "CBMkeyboard.h"

#ifdef _WINDOWS
#include <string.h>
#include <stdlib.h>
#include <Windows.h>
#include <stdio.h>
#include "WindowsKeyboard.h"
#include "WindowsTime.h"
#else
#include "cardkbdscan.h"
#ifdef ARDUINO_TEENSY41
#include "USBtoCBMkeyboard.h"
USBtoCBMkeyboard usbkbd;
#else // not ARDUINO_TEENSY41
#include "ble_keyboard.h"
#endif // not ARDUINO_TEENSY41
#endif // NOT _WINDOWS

bool CBMkeyboard::caps = false;

bool CBMkeyboard::heldToggle = false;

int CBMkeyboard::scan_codes[16] = { 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64 };

void CBMkeyboard::reset(CBMkeyboard::Model model)
{
  memset(scan_codes, model == C128 ? 88 : 64, sizeof(scan_codes));
  heldToggle = false;
}

void CBMkeyboard::ReadKeyboard(CBMkeyboard::Model model)
{
    // Vic-20, C64/128 share the same keyboard matrix
    // but wiring on Vic-20 mixes up the lines, and the row/col to scan code math is different
    // this code includes translation from a C64/128 scan code to a Vic-20 scan code
    int toVic20Row[8] = { 0, 1, 2, 7, 4, 5, 6, 3 };
    int toVic20Col[8] = { 7, 1, 2, 3, 4, 5, 6, 0 };

    static const byte toC64[24] = {
      64, 27, 16, 64, 59, 11, 24, 56,
      64, 40, 43, 64, 1, 19, 32, 8,
      64, 35, 44, 7, 7, 2, 2, 64
    };

#ifdef _WINDOWS
    static const int scan_codes_limit = sizeof(scan_codes) / sizeof(*scan_codes);

    WindowsKeyboard::get_scan_codes(scan_codes, scan_codes_limit);
    for (int i = 0; i < scan_codes_limit; ++i)
    {
        if (model != C128 && scan_codes[i] == 0x458) // C128 RESTORE, NO KEY
            scan_codes[i] = 0x440; // C64 RESTORE, NO KEY
        else if (model == C128)
          continue;
        else if (scan_codes[i] >= 88)
            scan_codes[i] = 64;
        else if (scan_codes[i] > 64)
            scan_codes[i] = toC64[scan_codes[i] - 64];
        
        if (model == VIC20 && scan_codes[i] < 64)
            scan_codes[i] = (toVic20Row[scan_codes[i] & 7] << 3) | toVic20Col[scan_codes[i] >> 3];
    }
    return;
#else // NOT _WINDOWS

    bool restartBLE = false;
#ifdef M5STACK
loop:
    const String upString = "15,7,88";
    const String dnString = "7,88";
    const String crString = "1,88";
    const String runString = "15,63,88";
    const String noString = "88";
    const String stopString = "63,88";
    static bool lastUp = false;
    static bool lastCr = false;
    static bool lastDn = false;
    static bool lastRun = false;
    static bool lastStop = false;

    M5.update();
#ifdef ARDUINO_M5STACK_CORES3
    static long pressed_time = 0;
    int touchcount = M5.Touch.getCount();
    bool a_pressed = false;
    bool b_pressed = false;
    bool c_pressed = false;
    bool a_held = false;
    bool b_held = false;
    bool c_held = false;
    if (touchcount > 0) {
      auto touchpoint = M5.Touch.getTouchPointRaw();
      if (touchpoint.y >= 230) {
        a_pressed = (touchpoint.x < 320 / 3);
        b_pressed = !a_pressed && (touchpoint.x < 320 * 2 / 3);
        c_pressed = !a_pressed && !b_pressed;

        if (lastCr && b_pressed && (millis() - pressed_time) >= 1000)
          b_held = true;
        if (!lastCr && b_pressed)
          pressed_time = millis();
        if (lastDn && c_pressed && (millis() - pressed_time) >= 1000)
          c_held = true;
        if (!lastDn && c_pressed)
          pressed_time = millis();
        }
        if (lastUp && a_pressed && (millis() - pressed_time) >= 1000)
          a_held = true;
        if (!lastUp && a_pressed)
          pressed_time = millis();
    }
#else // NOT ARDUINO_M5STACK_CORES3
    bool a_pressed = M5.BtnA.isPressed();
    bool b_pressed = M5.BtnB.isPressed();
    bool c_pressed = M5.BtnC.isPressed();
    bool a_held = M5.BtnA.pressedFor(1000);
    bool b_held = M5.BtnB.pressedFor(1000);
    bool c_held = M5.BtnC.pressedFor(1000);
#endif // NOT ARDUINO_M5STACK_CORES3    
    if (a_held)
      heldToggle = !c_pressed;
    if (b_held) {
      a_pressed = true;
      b_pressed = true;
    }
    if (a_held && c_held) {
      restartBLE = true;
      goto loop; // wait for release
    }
#endif // M5STACK

    String s;
#ifndef ARDUINO_TEENSY41
    ble_keyboard->ServiceConnection(restartBLE);
    s = ble_keyboard->Read();
    if (s.length() != 0)
        ;
    else
#endif
        if (CardKbd)
            s = CardKbdScanRead();
#ifndef ARDUINO_SUNTON_8048S070
#ifndef ARDUINO_TEENSY41
#ifndef ARDUINO_LILYGO_T_DISPLAY_S3
        else if (Serial2.available())
            s = Serial2.readString();
#endif
#endif
#endif
        else if (SerialDef.available())
            s = SerialDef.readString();
#ifdef M5STACK
        else if (lastRun && (lastRun = (a_pressed && b_pressed)) == false)
            s = noString;
        else if (lastUp && (lastUp = a_pressed && !c_pressed) == false)
            s = noString;
        else if (lastCr && (lastCr = b_pressed) == false)
            s = noString;
        else if (lastStop && (lastStop = c_held && !a_pressed) == false)
            s = noString;
        else if (lastDn && (lastDn = c_pressed && !a_pressed) == false)
            s = noString;
        else if ((lastRun = (a_pressed && b_pressed)) == true)
            s = runString;
        else if ((lastUp = a_pressed && !c_pressed) == true)
            s = upString;
        else if ((lastCr = b_pressed) == true)
            s = crString;
        else if ((lastStop = c_held && !a_pressed) == true)
            s = stopString;
        else if ((lastDn = c_pressed && !a_pressed) == true)
            s = dnString;
#endif    
#ifdef ARDUINO_TEENSY41
        else
            s = usbkbd.Read();
#endif
    if (s.length() == 0)
        return;

    //SerialDef.println(s);

    caps = false;
    int scan_lshift = (model == VIC20) ? 25 : 15;
    int scan_rshift = (model == VIC20) ? 38 : 52;

    unsigned src = 0;
    int dest = 0;
    int scan = 0;
    int len = 0;
    while (src < s.length() && dest < 16) {
        char c = s.charAt(src++);
        if (c >= '0' && c <= '9') {
            scan = scan * 10 + (c - '0');
            ++len;
        }
        else if (len > 0)
        {
            if (scan & 128) {
                caps = true;
                scan = 88;
            }
            int lobits = scan & 127;
            if (model != C128 && lobits >= 64 && lobits < 88)
            {
                scan = toC64[lobits - 64];
                for (int i = 0; i < dest; ++i)
                    if (scan_codes[i] == scan_lshift || scan_codes[i] == scan_rshift)
                        scan_codes[i] = 64;
                if (lobits == 83 || lobits == 85)
                    scan_codes[dest++] = scan_lshift;
            }
            if (model == VIC20 && scan < 64)
                scan = (toVic20Row[scan & 7] << 3) | toVic20Col[scan >> 3];
            if (model != C128 && scan > 64)
                scan = (scan & 0xFF80) | 64;
            if (model == C128 && scan == 64)
                scan = 88; // disable HELP key, some keyboard helpers still send 64 for no key // TODO: find and destroy bugs
            scan_codes[dest++] = scan;
            scan = 0;
            len = 0;
        }
    }
    while (dest < 16)
        scan_codes[dest++] = (model == C128) ? 88 : 64;

#endif // NOT _WINDOWS
}

void CBMkeyboard::waitKeysReleased(CBMkeyboard::Model model)
{
    bool keypressed;
    do {
        keypressed = false;
        CBMkeyboard::ReadKeyboard(model);
        for (int i = 0; !keypressed && i < 16; ++i)
            if ((scan_codes[i] & 127) != ((model == C128) ? 88 : 64))
                keypressed = true;
        delay(20);
    } while (keypressed);
}
