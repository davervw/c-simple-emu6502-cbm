/*
    BLE_commodore_keyboard_server.ino

    BLE Commodore Keyboard Server
    for c-simple-emu-cbm (C Portable Version)
    by David R. Van Wagner davevw.com
    Changes are open source, MIT License
    (Based on ESP32 BLE Arduino : BLE_server)

    Original comments:
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updates by chegewara
*/

// Synonyms ////////////////////////////
#ifdef ARDUINO_M5STACK_STICKC
#define ARDUINO_M5Stick_C
#endif

#ifdef ARDUINO_M5STACK_ATOMS3
#define ARDUINO_M5Stack_ATOMS3
#endif

#ifdef ARDUINO_M5STACK_CORE
#define ARDUINO_M5Stack_Core_ESP32
#endif
////////////////////////////////////////

#ifdef ARDUINO_M5Stack_ATOMS3
#include "M5Unified.hpp"
#endif
#ifdef ARDUINO_M5Stack_Core_ESP32
#include "M5Unified.hpp"
#endif
#ifdef ARDUINO_M5Stick_C
#include <Arduino.h>
#endif

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Wire.h>
#include "cardkbdscan.h"

// Commodore 64/128 BLE Keyboard Service
#define SERVICE_UUID        "65da11f8-dc46-4cd6-bdc9-ba862c4634f5"

// Commodore 64/128 BLE Keyboard Scan Characteristic
#define CHARACTERISTIC_UUID "1652b589-a0cc-4319-87fd-d80ccbd668f0"

BLECharacteristic *pCharacteristic;

void setup() {
#ifdef ARDUINO_M5Stick_C    
  pinMode(10, OUTPUT);
  digitalWrite(10, LOW);
#endif    
#ifdef ARDUINO_M5Stack_ATOMS3
  m5::M5Unified::config_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.external_display.atom_display = true;
  M5.begin(cfg);
#endif
#ifdef ARDUINO_M5Stack_Core_ESP32
  m5::M5Unified::config_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.external_display.atom_display = true;
  M5.begin(cfg);
#endif
  Serial.begin(115200);
  Serial.setTimeout(0); // so we don't wait for reads
  //Wire.begin(G2, G1, 100000UL); // ATOMS3
  //Wire.begin(32, 33, 100000UL); // M5Stick-C
  Wire.begin(SDA, SCL, 100000UL);
  for (int i=1; !CardKB && i<=5; ++i)
  {
    if (Wire.requestFrom((uint8_t)0x5F, (size_t)1, true) == 1)
    {
      CardKB = true;
      break;
    }
    delay(100);
  }
  if (!CardKB)
  {
    Wire.end();
    Serial2.end();
#ifdef ARDUINO_M5Stick_C    
    Serial2.begin(115200, SERIAL_8N1, 32, 33); // M5Stick-C
#endif
#ifdef ARDUINO_M5Stack_ATOMS3    
    Serial2.begin(115200, SERIAL_8N1, G2, G1); // M5AtomS3
#endif
#ifdef ARDUINO_M5Stack_Core_ESP32
    Serial2.begin(115200, SERIAL_8N1, G21, -1); // M5Core
#endif    
    Serial2.setTimeout(0); // so we don't wait for reads
  }

  Serial.println("Starting Commodore 64/128 BLE Keyboard Service");
  if (CardKB)
    Serial.println("CardKB or USB Serial");
  else
    Serial.println("HW Serial or USB Serial");
  BLEDevice::init("Commodore 64/128 BLE Keyboard Service");

  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );

  pCharacteristic->setValue("");
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("Started Commodore 64/128 BLE Keyboard Service");
}

void serviceLED()
{
  static long then = millis();
  static bool state = false;

  long now = millis();
  if ((now - then) >= 1000) {
    state = !state;
#ifdef ARDUINO_M5Stick_C    
    digitalWrite(10, state ? HIGH : LOW);
#endif
#ifdef ARDUINO_M5Stack_ATOMS3
    M5.Lcd.fillScreen(state ? BLUE : WHITE);
#endif
#ifdef ARDUINO_M5Stack_Core_ESP32
    M5.Lcd.fillScreen(state ? BLUE : WHITE);
#endif
    then = now;
  }
}

void loop() {
  serviceLED();
  String s = "";
  if (CardKB) 
    s = CardKbdScanRead();
  else
    s = Serial2.readString();
  if (s.length() == 0)
    s = Serial.readString();
  if (s.length() == 0)
    return;
  pCharacteristic->setValue(s.c_str());
  pCharacteristic->notify();
}
