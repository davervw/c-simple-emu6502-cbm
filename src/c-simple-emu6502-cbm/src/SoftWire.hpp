// Gemini 2026-07-04 mini-compatibility with TwoWire

#include <Arduino.h>

#define SOFTWIRE_BUFFER_SIZE 32

class SoftWire {
private:
    uint8_t _sda, _scl;
    
    // Transmission buffer (for writing)
    uint8_t _txBuffer[SOFTWIRE_BUFFER_SIZE];
    uint8_t _txLength = 0;
    
    // Reception buffer (for reading)
    uint8_t _rxBuffer[SOFTWIRE_BUFFER_SIZE];
    uint8_t _rxIndex = 0;
    uint8_t _rxLength = 0;

    // Low-level bit-banging helpers
    void sda_high() { pinMode(_sda, INPUT); }
    void sda_low()  { pinMode(_sda, OUTPUT); digitalWrite(_sda, LOW); }
    void scl_high() { pinMode(_scl, INPUT); delayMicroseconds(4); }
    void scl_low()  { pinMode(_scl, OUTPUT); digitalWrite(_scl, LOW); }

    void start() {
        sda_high();
        scl_high();
        sda_low();
        scl_low();
    }

    void stop() {
        sda_low();
        scl_high();
        sda_high();
    }

    bool writeByte(uint8_t byte) {
        for (uint8_t i = 0; i < 8; i++) {
            if (byte & 0x80) sda_high();
            else sda_low();
            scl_high();
            scl_low();
            byte <<= 1;
        }
        sda_high();
        scl_high();
        bool ack = (digitalRead(_sda) == LOW);
        scl_low();
        return ack;
    }

    uint8_t readByte(bool ack) {
        uint8_t byte = 0;
        sda_high();
        for (uint8_t i = 0; i < 8; i++) {
            scl_high();
            byte <<= 1;
            if (digitalRead(_sda)) byte |= 1;
            scl_low();
        }
        if (ack) sda_low();
        else sda_high();
        scl_high();
        scl_low();
        sda_high();
        return byte;
    }

public:
    // Default constructor (pins must be set via begin)
    SoftWire() : _sda(0), _scl(0) {}

    // Constructor with pins
    SoftWire(uint8_t sda, uint8_t scl) : _sda(sda), _scl(scl) {}

    // Initialize pins
    void begin() {
        pinMode(_sda, INPUT);
        pinMode(_scl, INPUT);
    }

    // Overload to allow setting pins at runtime if using default constructor
    void begin(uint8_t sda, uint8_t scl, unsigned long speed_ignored) {
        _sda = sda;
        _scl = scl;
        begin();
    }

    // Unused in master mode, added for interface compatibility
    void end() {}

    // Begin transmission to target device
    void beginTransmission(uint8_t address) {
        _txLength = 0;
        // Store 7-bit address shifted left with write bit (0)
        _txBuffer[_txLength++] = (address << 1); 
    }

    // End transmission and physically write data to the bus
    uint8_t endTransmission(bool sendStop = true) {
        if (_txLength == 0) return 4; // Other error

        start();
        
        // Send address byte
        if (!writeByte(_txBuffer[0])) {
            stop();
            return 2; // Received NACK on address
        }

        // Send data bytes
        for (uint8_t i = 1; i < _txLength; i++) {
            if (!writeByte(_txBuffer[i])) {
                stop();
                return 3; // Received NACK on data
            }
        }

        if (sendStop) {
            stop();
        }
        
        return 0; // Success
    }

    // Overload for default argument compatibility
    uint8_t endTransmission(void) {
        return endTransmission(true);
    }

    // Request bytes from a target device
    uint8_t requestFrom(uint8_t address, uint8_t quantity, bool sendStop = true) {
        if (quantity > SOFTWIRE_BUFFER_SIZE) {
            quantity = SOFTWIRE_BUFFER_SIZE;
        }

        start();
        
        // Send 7-bit address shifted left with read bit (1)
        if (!writeByte((address << 1) | 0x01)) {
            stop();
            return 0; 
        }

        _rxIndex = 0;
        _rxLength = quantity;

        for (uint8_t i = 0; i < quantity; i++) {
            // Send ACK for all bytes except the last one
            bool sendAck = (i < quantity - 1);
            _rxBuffer[i] = readByte(sendAck);
        }

        if (sendStop) {
            stop();
        }

        return _rxLength;
    }

    // Queue data for transmission
    size_t write(uint8_t data) {
        if (_txLength >= SOFTWIRE_BUFFER_SIZE) return 0;
        _txBuffer[_txLength++] = data;
        return 1;
    }

    size_t write(const uint8_t *data, size_t quantity) {
        size_t written = 0;
        for (size_t i = 0; i < quantity; i++) {
            if (write(data[i]) == 0) break;
            written++;
        }
        return written;
    }

    // Check available bytes to read
    int available() {
        return _rxLength - _rxIndex;
    }

    // Read byte from buffer
    int read() {
        if (_rxIndex < _rxLength) {
            return _rxBuffer[_rxIndex++];
        }
        return -1;
    }

    // Peek next byte in buffer
    int peek() {
        if (_rxIndex < _rxLength) {
            return _rxBuffer[_rxIndex];
        }
        return -1;
    }

    // Flush buffer
    void flush() {
        // No-op for master mode compatibility
    }
};
