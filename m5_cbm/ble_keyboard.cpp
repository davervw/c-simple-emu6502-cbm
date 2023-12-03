/*
 * ble_keyboard.cpp
 *
 * BLE Commodore Keyboard Server
 * for c-simple-emu-cbm (C Portable Version)
 * by David R. Van Wagner davevw.com
 * Changes are open source, MIT License
 * (Revised from example ESP32 BLE Arduino : BLE_server)
 *
 * Original comments:
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 */

#include "M5Core.h"
#include "ble_keyboard.h"
#include "BLEDevice.h"

// The remote service we wish to connect to.
static BLEUUID serviceUUID("65da11f8-dc46-4cd6-bdc9-ba862c4634f5"); // Commodore 64/128 BLE Keyboard Service
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("1652b589-a0cc-4319-87fd-d80ccbd668f0"); // Commodore 64/128 BLE Keyboard Scan Characteristic

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

class KeyboardQueue
{
private:
  int queue_size = 5;
  int head;
  int tail;
  String* queue;

public:
  KeyboardQueue()
  {
    head = tail = 0;
    queue = new String[queue_size];
  }

  ~KeyboardQueue()
  {
    delete [] queue;
  }

  boolean Enqueue(String s)
  {
    int next_head = (head + 1) % queue_size;
    if (next_head == tail)
      return false;
    queue[head] = s;
    head = next_head;
    return true;
  }

  String Dequeue()
  {
    if (head == tail)
      return "";
    String s = queue[tail];
    tail = (tail + 1) % queue_size;
    return s;
  }
};

KeyboardQueue* kbdqueue = new KeyboardQueue();

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    M5Serial.write("BLE received: ");
    M5Serial.write(pData, length);
    if (length < 1 || pData[length-1] != '\n')
      M5Serial.println();
    kbdqueue->Enqueue(String(pData, length));
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    M5Serial.println("onDisconnect");
  }
};

static bool connectToServer() {
    M5Serial.print("Forming a connection to ");
    M5Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    M5Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    M5Serial.println(" - Connected to server");
    pClient->setMTU(517); //set client to request maximum MTU from server (default is 23 otherwise)
  
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      M5Serial.print("Failed to find our service UUID: ");
      M5Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    M5Serial.println(" - Found our service");

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      M5Serial.print("Failed to find our characteristic UUID: ");
      M5Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    M5Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      M5Serial.print("The characteristic value was: ");
      M5Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    M5Serial.print("BLE Advertised Device found: ");
    M5Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


BLE_Keyboard::BLE_Keyboard()
{
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
} // End of setup.

// This is the Arduino main loop function.
void BLE_Keyboard::ServiceConnection() {
  static unsigned long timer_then = micros();
  const unsigned long timeout = 1000000;

  unsigned long timer_now = micros();
  unsigned long elapsed_micros = micros() - timer_then;
  if (elapsed_micros < timeout)
    return;
  timer_then = timer_now;

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      M5Serial.println("We are now connected to the BLE Server.");
    } else {
      M5Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    // String newValue = "Time since boot: " + String(millis()/1000);
    // M5Serial.println("Setting new characteristic value to \"" + newValue + "\"");
    
    // // Set the characteristic's value to be the array of bytes that is actually a string.
    // pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }
} // End of loop

String BLE_Keyboard::Read()
{
  String s = kbdqueue->Dequeue();
  if (s == 0)
    return "";
  return s;
}
