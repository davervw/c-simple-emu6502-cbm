//////////////////////////////////////////////////////////////////////
// autoblehid.cpp
//
// use_BLEHID_esp32 - auto pairing with BLE Keyboard, Mouse, Gamepad
// Copyright (c) 2026 David R. Van Wagner
//
// MIT LICENSE
//
// https://davevw.com
// https://github.com/davervw
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
#include "autoblehid.h"
#include "blehid.h"

AutoBleHid AUTOBLEHID;

// forward declaration
static void doScan();

static bool do_connect = false;
static void (*_hidReport)(size_t length, uint8_t* data, bool isCBM) = nullptr;

static void scanResult(bool found)
{
    // Serial.println("Scanning finished.");
#if (CORE_DEBUG_LEVEL >= 3)    
    Serial.printf(found ? "FOUND %s\n" : "NOT FOUND\n", BLEHID.isKeyboard() ? "Keyboard" : BLEHID.isGamePad() ? "Gamepad" : "");
#endif    
    if (found)
        do_connect = true;
}

static void doScan()
{
#if (CORE_DEBUG_LEVEL >= 3)    
    Serial.println("Scanning...");
#endif
    BLEHID.scan(&scanResult, 0);
}

static void onDisconnected()
{
    Serial.println("Disconnected");
    doScan();
}

static void doConnect()
{
#if (CORE_DEBUG_LEVEL >= 3)    
    Serial.println("Connecting...");
#endif
    if (BLEHID.connect(&onDisconnected))
    {
#if (CORE_DEBUG_LEVEL >= 3)    
        Serial.println("Connected to device.");
#endif
        auto map = BLEHID.getHIDmap();
#if (CORE_DEBUG_LEVEL >= 3)    
        Serial.printf("HID map size = %ld\n", (long)map.size());
#endif
        if (map.size() == 0)
            Serial.println("Failed to receive HID map");
        if (BLEHID.listenReports(_hidReport))
        {
#if (CORE_DEBUG_LEVEL >= 3)    
            Serial.println("Listening for reports");
#endif
        }
        else
        {
            Serial.println("Failed to listen for reports");
        }
    }
}

void AutoBleHid::begin(void (*hidReport)(size_t length, uint8_t* data, bool isCBM))
{
    _hidReport = hidReport;
    BLEHID.init();
    doScan();
}

void AutoBleHid::update()
{
    if (do_connect)
    {
        do_connect = false;
        doConnect();
    }
}
