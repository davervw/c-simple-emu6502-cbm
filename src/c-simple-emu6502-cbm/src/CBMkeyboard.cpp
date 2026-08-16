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
extern USBtoCBMkeyboard usbkbd;
#else // not ARDUINO_TEENSY41
#include "autoblehid.h"
#include "blehid.h"
#endif // not ARDUINO_TEENSY41
#endif // NOT _WINDOWS
#ifdef M5TAB5
#include "tab5keymatrix.h"
#include "tab5keystoc128.h"
#else //!M5TAB5
#include <queue>
#include "HIDtoCBMkeyboard.h"
#endif //!M5TAB5

bool CBMkeyboard::caps = false;

bool CBMkeyboard::heldToggle = false;

int CBMkeyboard::scan_codes[16] = { 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64 };

uint8_t CBMkeyboard::joystick_vic20_1 = 255;
uint8_t CBMkeyboard::joystick_vic20_2 = 255;
uint8_t CBMkeyboard::joystick_c64_1 = 255;
uint8_t CBMkeyboard::joystick_c64_2 = 255;

#ifndef ARDUINO_TEENSY41
#ifndef M5TAB5
#ifndef _WINDOWS
static bool initAUTOBLEHID = false;
#endif // !_WINDOWS
#endif // !M5TAB5
#endif // !ARDUINO_TEENSY41

#ifndef M5TAB5
#ifndef _WINDOWS
static HIDtoCBMkeyboard hidcbm;
std::queue<String> scancodeQueue;

void decodeHatButton(uint8_t hat, uint8_t button)
{
    const int c64_up = 1;
    const int c64_down = 2;
    const int c64_left = 4;
    const int c64_right = 8;
    const int c64_a = 16;

    uint8_t data = 255;
    switch (hat)
    {
        case 1: data -= c64_up; break;
        case 2: data -= c64_up + c64_right; break;
        case 3: data -= c64_right; break;
        case 4: data -= c64_right + c64_down; break;
        case 5: data -= c64_down; break;
        case 6: data -= c64_left + c64_down; break;
        case 7: data -= c64_left; break;
        case 8: data -= c64_left + c64_up; break;
    }
    if ((button & 1) == 1)
        data -= c64_a;

    static bool last_b = false;
    static int joystick_i = 2;
    bool b = ((button & 2) == 2);
    if (b != last_b && !b) // when b released
        joystick_i = 3 - joystick_i; // swap joysticks
    last_b = b;

    switch (joystick_i)
    {
        case 1: CBMkeyboard::joystick_c64_1 = data; break;
        case 2: CBMkeyboard::joystick_c64_2 = data; break;
    }

    // all over again, but this time for Vic-20

    const int vic20_up = 4;
    const int vic20_down = 8;
    const int vic20_left = 16;
    const int vic20_right = 128;
    const int vic20_a = 32;

    data = 255;
    switch (hat)
    {
        case 1: data -= vic20_up; break;
        case 2: data -= vic20_up + vic20_right; break;
        case 3: data -= vic20_right; break;
        case 4: data -= vic20_right + vic20_down; break;
        case 5: data -= vic20_down; break;
        case 6: data -= vic20_left + vic20_down; break;
        case 7: data -= vic20_left; break;
        case 8: data -= vic20_left + vic20_up; break;
    }
    if ((button & 1) == 1)
        data -= vic20_a;

    CBMkeyboard::joystick_vic20_1 = (data & 0x7f) | 0x80;
    CBMkeyboard::joystick_vic20_2 = 0x7f | (data & 0x80);
}

void hidReport(size_t len, uint8_t *data, bool isCBM)
{
    bool isKeyboard = (len == 8); //BLEHID.isKeyboard();
    if (isCBM) {
#ifdef ARDUINO_TEENSY41
        String s = "";
        for (int i=0; i<len; ++i)
            s += (char)data[i];
#else        
        String s = String(data, len);
#endif        
        if (s.length() != 0)
            scancodeQueue.push(s);
    } else if (isKeyboard) {
        hidcbm.OnKeyData(len, data);
        String s = hidcbm.Read();
        if (s.length() != 0)
            scancodeQueue.push(s);
    } else {
        // other HID
        int di = -1;
        int ai = -1;
        switch (len)
        {
            case 4: di = 3; ai = 0; break; // Kano Pixel Kit BLE
            case 7: di = 6; ai = 0; break; // MiniJoyC BLE
            case 16: di = 12; ai = 13; break; // XINPUT (Xbox BLE)
        }
        if (di >= 0 && di < len && ai >= 0 && ai < len)
            decodeHatButton(data[di], data[ai]);
    }
}

static bool tryByteRead(String s)
{
  uint8_t buffer[8];
  static char hex[] = "0123456789ABCDEF";

  int i=0;
  auto len = s.length();
  while (true)
  {
    if (3*i >= s.length() || s[3*i] == '\n')
     break;
    auto hi_p = strchr(hex, s[3*i]);
    auto lo_p = strchr(hex, s[3*i+1]);
    if (hi_p == nullptr || lo_p == nullptr)
      return false;
    if (3*i+2 < s.length() && s[3*i+2] != ' ')
        return false;
    auto hi = hi_p - &hex[0];
    auto lo = lo_p - &hex[0];
    if (hi < 0 || hi > 15 || lo < 0 || lo > 15)
      return false;
    buffer[i++] = (hi << 4) | lo;
  }
  hidReport(sizeof(buffer), &buffer[0], false);
  return true;
}
#endif //!_WINDOWS
#endif //!M5TAB5

void CBMkeyboard::reset(CBMkeyboard::Model model)
{
#ifndef ARDUINO_TEENSY41
#ifndef M5TAB5
#ifndef _WINDOWS
  if (!initAUTOBLEHID)
    AUTOBLEHID.begin(hidReport);
#endif
#endif    
#endif    
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

#ifndef ARDUINO_TEENSY41
    bool restartBLE = false;
#endif    
#ifdef M5STACK
loop:
    const String upString = "15,7,88";
    const String dnString = "7,88";
    const String crString = "1,88";
    const String runString = "15,63,88";
    const String noString = "88";
    const String stopString = "63,88";
    const String ltString = "15,2,88";
    const String rtString = "2,88";
    const String homeString = "51,88";
    const String delString = "0,88";
    const String spaceString = "60,88";
    static bool lastUp = false;
    static bool lastCr = false;
    static bool lastDn = false;
    static bool lastRun = false;
    static bool lastStop = false;
    static bool lastLt = false;
    static bool lastRt = false;
    static bool lastHome = false;
    static bool lastDel = false;
    static bool lastSpace = false;

    M5.update();
#if (defined(ARDUINO_M5STACK_CORES3) || defined(M5TAB5))
    static long pressed_time = 0;
    int touchcount = M5.Touch.getCount();
    bool a_pressed = false;
    bool b_pressed = false;
    bool c_pressed = false;
    bool l_pressed = false;
    bool r_pressed = false;
    bool t_pressed = false;
    bool one_pressed = false;
    bool two_pressed = false;
    bool a_held = false;
    bool b_held = false;
    bool c_held = false;
    if (touchcount > 0) {
      auto touchpoint = M5.Touch.getTouchPointRaw();
      auto width = M5.Display.width();
      auto height = M5.Display.height();
#ifdef M5TAB5
      auto x = width - touchpoint.y;
      auto y = touchpoint.x;
#else
      auto x = touchpoint.x;
      auto y = touchpoint.y;
#endif
      if (y >= height * 9 / 10) {
          a_pressed = (x < width / 3);
          b_pressed = !a_pressed && (x < width * 2 / 3);
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
      if (y < height / 10) {
        one_pressed = (x < width / 3);
        t_pressed = (x >= width / 3 && x <= width * 2 / 3);
        two_pressed = (x > width * 2 / 3);
      }
      if (y >= height / 3 && y <= height * 2 / 3) {
        l_pressed = x <= width / 10;
        r_pressed = x >= width * 9 / 10;
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

    String s = "";
#ifndef ARDUINO_TEENSY41
#ifndef M5TAB5
    AUTOBLEHID.update();
    if (!scancodeQueue.empty()) {
        s = scancodeQueue.front();
        scancodeQueue.pop();
    }
    if (s.length() != 0)
        ;
    else
#endif    
#endif
        if (CardKbd)
            s = CardKbdScanRead();
#ifndef ARDUINO_SUNTON_8048S070
#ifndef ARDUINO_TEENSY41
#ifndef ARDUINO_LILYGO_T_DISPLAY_S3
        if (s.length() == 0 && Serial2.available())
            s = Serial2.readString();
#endif
#endif
#endif
        if (s.length() == 0 && SerialDef.available())
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
        else if (lastLt & (lastLt = l_pressed) == false)
            s = noString;
        else if (lastRt & (lastRt = r_pressed) == false)
            s = noString;
        else if (lastHome && (lastHome = t_pressed) == false)
            s = noString;
        else if (lastDel && (lastDel = one_pressed) == false)
            s = noString;
        else if (lastSpace && (lastSpace = two_pressed) == false)
            s = noString;
        else if (lastLt = l_pressed)
            s = ltString;
        else if (lastRt = r_pressed)
            s = rtString;
        else if (lastHome = t_pressed)
            s = homeString;
        else if (lastDel = one_pressed)
            s = delString;
        else if (lastSpace = two_pressed)
            s = spaceString;
#endif    
#ifdef ARDUINO_TEENSY41
        else if (Serial1.available() > 0) {
            do {
                int byte = Serial1.read();
                if (byte == -1)
                    continue;
                s += (char)byte;
            } while (s[s.length()-1] != '\n');
            if (s.length() == 49) {
                tryByteRead(s);
            }
            s = "";      
        }
        if (s.length() == 0)
            s = usbkbd.Read();
#endif
#ifdef M5TAB5
        else
        {
            Tab5KeyMatrix::update();
            m5::unit::tab5_keyboard::key_status_bits_t keys;
            if (Tab5KeyMatrix::check_key_change(keys))
            {
                uint16_t *c128_keys;
                uint8_t c128_size;
                tab5_key_matrix_to_c128(keys, c128_keys, c128_size);
                char sb[256];
                char *p = &sb[0];
                *p = '\0';
                for (int i = 0; i < c128_size; ++i)
                {
                    auto key = c128_keys[i];
                    if (key == 88)
                        break;
                    p += sprintf(p, "%d,", key);
                }
                sprintf(p, "88\n");
                s = String(sb);
            }
        }
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
    while (src < s.length() && dest < 16)
    {
        char c = s.charAt(src++);
        if (c >= '0' && c <= '9')
        {
            scan = scan * 10 + (c - '0');
            ++len;
        }
        else if (len > 0)
        {
            if (scan & 128)
            {
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
                scan = 88; // disable HELP key, some keyboard helpers (e.g. M5Stick-C) still send 64 for no key // TODO: find and destroy bugs
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
