# c-simple-emu-cbm #

Here is a simple Commodore 64 and 6502 Emulator I wrote from scratch, ported to C from my C# [simple-emu-c64](https://github.com/davervw/simple-emu-c64) project.  Runs from the serial port (in your favorite terminal set echo on, local edit on, implicit CR when LF, 11520 bps).  More details may be relevant from the [simple-emu-c64](https://github.com/davervw/simple-emu-c64) project.

The portable version has been tested with multiple platforms.

This [Arduino](https://github.com/davervw/c-simple-emu6502-cbm/tree/arduino) version is branched from [master](https://github.com/davervw/c-simple-emu6502-cbm/tree/master) and tested specifically with:

* [Arduino Due](https://store-usa.arduino.cc/products/arduino-due) (ARM ATSAM3X8E)
* [Teensy 3.5](https://www.pjrc.com/store/teensy35.html)
* [Teensy 4.1](https://www.pjrc.com/store/teensy41.html)
* Espressif [ESP32-WROOM-32](https://www.espressif.com/en/products/modules/esp-wroom-32/overview)

Only CHRIN/CHROUT are hooked, so no, it won't run your games, only maybe simple ASCII text adventures or such.  Sorry no PETSCII either.

![circle.bas](https://github.com/davervw/c-simple-emu6502-cbm/raw/arduino/circle.png)
