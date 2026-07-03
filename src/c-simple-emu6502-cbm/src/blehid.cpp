//////////////////////////////////////////////////////////////////////
// blehid.cpp
//
// use_BLEHID_esp32 - auto pairing with BLE Keyboard, Mouse, Gamepad
// Copyright (c) 2026 David R. Van Wagner
//
// MIT LICENSE
//
// https://davevw.com
// https://github.com/davervw
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
#include <map>
#include <NimBLEDevice.h>
#include "blehid.h"

cBLEHID BLEHID;

static bool _isconnected = false;
static bool _isscanning = false;
static bool _iskeyboard = false;
static bool _ismouse = false;
static bool _isgamepad = false;

static const NimBLEAdvertisedDevice *_device = nullptr;
static NimBLEScan *_scan = nullptr;
static NimBLEClient *_client = nullptr;

static void (*_scanResult)(bool found) = nullptr;
static void (*_hidReport)(size_t len, uint8_t *data, bool isCBM);
static void (*_disconnected)();

static const NimBLEUUID HIDSERVICEUUID = NimBLEUUID("1812");
static const NimBLEUUID CBMSERVICEUUID = NimBLEUUID("65da11f8-dc46-4cd6-bdc9-ba862c4634f5");
static const NimBLEUUID CBMCHARACTERISTICUUID = NimBLEUUID("1652b589-a0cc-4319-87fd-d80ccbd668f0");
// ------------------------------------------------------------------------------

class ClientCallbacks : public NimBLEClientCallbacks
{
public:
    explicit ClientCallbacks() {}
    void onConnect(NimBLEClient *pClient) override;
    void onDisconnect(NimBLEClient *pClient, int reason) override;
    void onPassKeyEntry(NimBLEConnInfo &connInfo) override;
    void onConfirmPasskey(NimBLEConnInfo &connInfo, uint32_t passkey) override;
    void onAuthenticationComplete(NimBLEConnInfo &connInfo) override;
};
static ClientCallbacks clientCallbacks;

void ClientCallbacks::onConnect(NimBLEClient *)
{
    _isconnected = true;
}

void ClientCallbacks::onDisconnect(NimBLEClient *, int reason)
{
    _isconnected = false;
    _iskeyboard = false;
    _isgamepad = false;
    _device = nullptr;
#if (CORE_DEBUG_LEVEL >= 3)    
    Serial.printf("Disconnected, reason=%d\n", reason);
#endif
    // clearMonitorSubscriptions();
    if (_disconnected != nullptr)
        _disconnected();
}

void ClientCallbacks::onPassKeyEntry(NimBLEConnInfo &connInfo)
{
#if (CORE_DEBUG_LEVEL >= 3)    
    Serial.println("Passkey requested; injecting default test value.");
#endif
    NimBLEDevice::injectPassKey(connInfo, 123456);
}

void ClientCallbacks::onConfirmPasskey(NimBLEConnInfo &connInfo, uint32_t passkey)
{
#if (CORE_DEBUG_LEVEL >= 3)    
    Serial.print("Confirming passkey ");
    Serial.println(passkey);
#endif
    NimBLEDevice::injectConfirmPasskey(connInfo, true);
}

void ClientCallbacks::onAuthenticationComplete(NimBLEConnInfo &connInfo)
{
    if (!connInfo.isEncrypted())
    {
#if (CORE_DEBUG_LEVEL >= 3)    
        Serial.println("Authentication complete, but link is not encrypted.");
#endif
        NimBLEClient *client = NimBLEDevice::getClientByHandle(connInfo.getConnHandle());
        if (client != nullptr)
        {
            client->disconnect();
        }
    }
    else
    {
#if (CORE_DEBUG_LEVEL >= 3)    
        Serial.println("Authentication complete.");
#endif
    }
}

// ------------------------------------------------------------------------------

class ScanCallbacks : public NimBLEScanCallbacks
{
public:
    explicit ScanCallbacks() {}
    void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override;
    void onScanEnd(const NimBLEScanResults &results, int reason) override;
};
static ScanCallbacks scanCallbacks;

void ScanCallbacks::onResult(const NimBLEAdvertisedDevice *advertisedDevice)
{
    // Serial.println("Found something");

    if (_device != nullptr)
        return;

    if (!advertisedDevice->isConnectable())
        return;

    if (!advertisedDevice->haveServiceUUID())
        return;

    // Only accept devices advertising the HID service (0x1812) or custom service
    bool isHID = advertisedDevice->isAdvertisingService(HIDSERVICEUUID);
    bool isCBM = advertisedDevice->isAdvertisingService(CBMSERVICEUUID);
    if (!isHID && !isCBM)
        return;

    if (!isCBM && !advertisedDevice->haveAppearance())
        return;

    auto appearance = advertisedDevice->getAppearance();
#if (CORE_DEBUG_LEVEL >= 3)    
    Serial.printf("Found appearance %04x\n", appearance);
#endif    
    if (appearance == 0x3c1 || isCBM)
    {
        _device = advertisedDevice;
        _iskeyboard = true;
        // Serial.println("Found keyboard");
    }
    else if (appearance == 0x3c4)
    {
        _device = advertisedDevice;
        _isgamepad = true;
        // Serial.println("Found gamepad");
    }

    if (_scanResult != nullptr)
        _scanResult(true);
    _scan->stop();
    _isscanning = false;
}

void ScanCallbacks::onScanEnd(const NimBLEScanResults &results, int reason)
{
    _isscanning = false;
    // Serial.println("Scanning finished.");

    if (_device != nullptr)
        return;

    if (_scanResult != nullptr)
        _scanResult(false);
}

void cBLEHID::init()
{
    NimBLEDevice::init("BLEHID");
    NimBLEDevice::setPower(3);
    NimBLEDevice::setSecurityAuth(true, false, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
}

void cBLEHID::scan(void (*scanResult)(bool found), uint32_t durationMs)
{
    if (_isscanning)
        return;

    _scanResult = scanResult;

    _scan = NimBLEDevice::getScan();
    _scan->setScanCallbacks(&scanCallbacks, true);
    _scan->setInterval(100);
    _scan->setWindow(100);
    _scan->setActiveScan(true);

    _isscanning = true;

    // Serial.println("Scanning...");

    _scan->start(durationMs, false, true);
}

bool cBLEHID::connect(void (*disconnected)())
{
    _disconnected = disconnected;

    NimBLEDevice::getScan()->stop();

    auto address = _device->getAddress();

    if (NimBLEDevice::getCreatedClientCount())
    {
        _client = NimBLEDevice::getClientByPeerAddress(address);
        if (_client != nullptr && _client->isConnected())
        {
#if (CORE_DEBUG_LEVEL >= 3)    
            Serial.println("Already connected.");
#endif
            return true;
        }
        if (_client == nullptr)
        {
            _client = NimBLEDevice::getDisconnectedClient();
        }
    }

    if (_client == nullptr)
    {
        _client = NimBLEDevice::createClient();
        if (_client == nullptr)
        {
#if (CORE_DEBUG_LEVEL >= 3)    
            Serial.println("Failed to create client.");
#endif
            return false;
        }
        _client->setClientCallbacks(&clientCallbacks, false);
        _client->setConnectionParams(12, 12, 0, 150);
        _client->setConnectTimeout(5000);
    }

    if (!_client->connect(address, false))
    {
#if (CORE_DEBUG_LEVEL >= 3)    
        Serial.println("Connect failed.");
#endif
        return false;
    }

    return true;
}

void cBLEHID::disconnect()
{
    if (!_isconnected)
        return;
    _client->disconnect();
    _client = nullptr;
    _isconnected = false;
    _device = nullptr;
    _iskeyboard = false;
    _isgamepad = false;
}

static std::vector<uint16_t> _subscribedHandles;

static NimBLERemoteCharacteristic *findHidCharacteristicByHandle(uint16_t handle)
{
    if (!_isconnected)
    {
        return nullptr;
    }

    NimBLERemoteService *hid = _client->getService(HIDSERVICEUUID);
    if (hid == nullptr)
        hid = _client->getService(CBMSERVICEUUID);
    if (hid == nullptr)
        return nullptr;

    const auto &chars = hid->getCharacteristics(true);
    for (auto *chr : chars)
    {
        if (chr != nullptr && chr->getHandle() == handle)
        {
            return chr;
        }
    }

    return nullptr;
}

static void clearMonitorSubscriptions()
{
    if (!_isconnected)
    {
        _subscribedHandles.clear();
        return;
    }

    for (uint16_t handle : _subscribedHandles)
    {
        NimBLERemoteCharacteristic *chr = findHidCharacteristicByHandle(handle);
        if (chr != nullptr)
        {
            chr->unsubscribe(true);
        }
    }

    _subscribedHandles.clear();
}

static bool isHidInputReportCharacteristic(NimBLERemoteCharacteristic *characteristic, uint8_t *reportIdOut)
{
    if (characteristic == nullptr)
    {
        return false;
    }

    if (!characteristic->getUUID().equals(NimBLEUUID((uint16_t)0x2A4D)))
    {
        return false;
    }

    const auto &descriptors = characteristic->getDescriptors(true);
    for (auto *desc : descriptors)
    {
        if (desc == nullptr)
        {
            continue;
        }
        if (!desc->getUUID().equals(NimBLEUUID((uint16_t)0x2908)))
        {
            continue;
        }

        std::string value = desc->readValue();
        if (value.size() < 2)
        {
            continue;
        }

        const uint8_t reportId = static_cast<uint8_t>(value[0]);
        const uint8_t reportType = static_cast<uint8_t>(value[1]);
        if (reportIdOut != nullptr)
        {
            *reportIdOut = reportId;
        }
        return reportType == 1;
    }

    return false;
}

#if (CORE_DEBUG_LEVEL >= 3)    
// Definition of possible HID device components
enum HidDeviceType { 
    HID_UNKNOWN, 
    HID_KEYBOARD, 
    HID_MOUSE, 
    HID_MEDIA_KEYS, 
    HID_GAMEPAD 
};

// Global maps for the callback to use
std::map<uint16_t, HidDeviceType> handleToTypeMap;
std::map<uint16_t, uint8_t> handleToReportIdMap;

static void discoverAndClassifyCharacteristics(NimBLERemoteService* pService) {
    auto characteristicMap = pService->getCharacteristics();
    
    for (auto pChr : characteristicMap) {
        // Match only standard generic HID Report characteristics
        if (pChr->getUUID() == NimBLEUUID((uint16_t)0x2A4D)) {
            uint16_t currentHandle = pChr->getHandle();
            
            // Fetch the Report Reference descriptor (0x2908)
            NimBLERemoteDescriptor* pDesc = pChr->getDescriptor(NimBLEUUID((uint16_t)0x2908));
            
            if (pDesc != nullptr) {
                std::string descVal = pDesc->readValue();
                
                if (descVal.length() >= 2) {
                    uint8_t reportId   = descVal[0]; // Byte 0 = Report ID
                    uint8_t reportType = descVal[1]; // Byte 1 = Type (1=Input, 2=Output)
                    
                    HidDeviceType identifiedType = HID_UNKNOWN;

                    if (reportType == 1) { // Process INPUT reports only
                        // Match these IDs to your device's specific HID descriptor map:
                        if (reportId == 0x01) {
                            identifiedType = HID_KEYBOARD;
                            Serial.printf("Handle 0x%04X -> KEYBOARD (Report ID: %d)\n", currentHandle, reportId);
                        } 
                        else if (reportId == 0x02) { 
                            identifiedType = HID_MOUSE;
                            Serial.printf("Handle 0x%04X -> MOUSE (Report ID: %d)\n", currentHandle, reportId);
                        } 
                        else if (reportId == 0x03) {
                            identifiedType = HID_GAMEPAD;
                            Serial.printf("Handle 0x%04X -> GAMEPAD (Report ID: %d)\n", currentHandle, reportId);
                        }
                        else {
                            // High Report IDs or multi-media maps usually contain consumer controls
                            identifiedType = HID_MEDIA_KEYS;
                            Serial.printf("Handle 0x%04X -> MEDIA KEYS (Report ID: %d)\n", currentHandle, reportId);
                        }
                    }
                    
                    // Cache the maps for instant callback lookup
                    handleToTypeMap[currentHandle] = identifiedType;
                    handleToReportIdMap[currentHandle] = reportId;
                }
            }
        }
    }
}
#endif

static void notifyCallback(NimBLERemoteCharacteristic *characteristic, uint8_t *data, size_t length, bool isNotify)
{
#if (CORE_DEBUG_LEVEL >= 3)    
    Serial.printf("%s:", characteristic->getUUID().toString().c_str());

    uint16_t currentHandle = characteristic->getHandle();

    HidDeviceType deviceType = HID_UNKNOWN;
    uint8_t reportId = 0;

    if (handleToTypeMap.find(currentHandle) != handleToTypeMap.end()) {
        deviceType = handleToTypeMap[currentHandle];
        reportId = handleToReportIdMap[currentHandle];
        switch (deviceType)
        {
            case HID_KEYBOARD: Serial.print("Keyboard: "); break;
            case HID_MOUSE: Serial.print("Mouse: "); break;
            case HID_MEDIA_KEYS: Serial.print("Media Keys: "); break;
            case HID_GAMEPAD: Serial.print("Gamepad: "); break;
        }
    }
#endif

    bool isCBM = characteristic->getUUID() == CBMCHARACTERISTICUUID;
    _hidReport(length, data, isCBM);
}

bool cBLEHID::listenReports(void (*hidReport)(size_t len, uint8_t *data, bool isCBM))
{
    if (!isConnected())
        return false;

    clearMonitorSubscriptions();

    _hidReport = hidReport;

    NimBLERemoteService *hid = _client->getService(HIDSERVICEUUID);
    if (hid == nullptr)
        hid = _client->getService(CBMSERVICEUUID);
    if (hid == nullptr)
    {
#if (CORE_DEBUG_LEVEL >= 3)    
        Serial.println("HID service not found.");
#endif
        return false;
    }

#if (CORE_DEBUG_LEVEL >= 3)    
    discoverAndClassifyCharacteristics(hid);
#endif

    bool subscribed = false;
    const auto &chars = hid->getCharacteristics(true);
    for (auto *chr : chars)
    {
        if (chr == nullptr)
        {
            continue;
        }

        uint8_t reportId = 0;
        const bool isBootKeyboard = chr->getUUID().equals(NimBLEUUID((uint16_t)0x2A22));
        const bool isCbmKeyboard = chr->getUUID().equals(CBMCHARACTERISTICUUID);
        const bool isInputReport = isHidInputReportCharacteristic(chr, &reportId);
        if (!isBootKeyboard && !isCbmKeyboard && !isInputReport)
        {
            continue;
        }

#if (CORE_DEBUG_LEVEL >= 3)    
        Serial.print("Inspecting ");
        Serial.print(chr->getUUID().toString().c_str());
        Serial.print(" notify=");
        Serial.print(chr->canNotify() ? "y" : "n");
        Serial.print(" indicate=");
        Serial.println(chr->canIndicate() ? "y" : "n");
#endif

        if (chr->canNotify())
        {
            if (chr->subscribe(true, notifyCallback))
            {
                subscribed = true;
                _subscribedHandles.push_back(chr->getHandle());
#if (CORE_DEBUG_LEVEL >= 3)    
                Serial.print("Subscribed notify ");
                Serial.println(chr->getUUID().toString().c_str());
#endif                
            }
            else
            {
#if (CORE_DEBUG_LEVEL >= 3)    
                Serial.print("Failed subscribe notify ");
                Serial.println(chr->getUUID().toString().c_str());
#endif
            }
        }
        else if (chr->canIndicate())
        {
            if (chr->subscribe(false, notifyCallback))
            {
                subscribed = true;
                _subscribedHandles.push_back(chr->getHandle());
#if (CORE_DEBUG_LEVEL >= 3)    
                Serial.print("Subscribed indicate ");
                Serial.println(chr->getUUID().toString().c_str());
#endif
            }
            else
            {
#if (CORE_DEBUG_LEVEL >= 3)    
                Serial.print("Failed subscribe indicate ");
                Serial.println(chr->getUUID().toString().c_str());
#endif
            }
        }
    }

    if (!subscribed)
    {
#if (CORE_DEBUG_LEVEL >= 3)    
        Serial.println("No HID characteristics were subscribable.");
#endif
    }
    return subscribed;
}

std::vector<uint8_t> cBLEHID::getHIDmap()
{
    NimBLERemoteCharacteristic *mapChar = nullptr;
    NimBLERemoteService *hid = nullptr;
    static std::vector<uint8_t> result = {};
    result.clear();

    if (!isConnected())
    {
        return result;
    }

    hid = _client->getService(HIDSERVICEUUID);
    if (hid == nullptr)
        return result;

    mapChar = hid->getCharacteristic(NimBLEUUID((uint16_t)0x2A4B));
    if (mapChar == nullptr)
    {
        return result;
    }

    std::string value = mapChar->readValue();
    if (value.empty())
    {
        return result;
    }

    auto byteArray = reinterpret_cast<const uint8_t*>(value.data());
    result = std::vector<uint8_t>(byteArray, byteArray + value.size());
    return result;
}

bool cBLEHID::isConnected() { return _isconnected; }
bool cBLEHID::isScanning() { return _isscanning; }
bool cBLEHID::isKeyboard() { return _iskeyboard; }
bool cBLEHID::isGamePad() { return _isgamepad; }
