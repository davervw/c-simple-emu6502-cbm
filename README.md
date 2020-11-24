# Teensy C64 #

Here is a simple Commodore 64 and 6502 Emulator I wrote from scratch, ported to C from my C# [simple-emu-c64](https://github.com/davervw/simple-emu-c64) project.  Takes input from serial port (115200n81) and displays ASCII output.  

* Development environment is Arduino IDE (1.8.5 or later)
* Tested with Teensy 4.1 and ILI9341 LCD screen
* Displays C64 color text screen to LCD with PETSCII supporting two character sets (default without lowercase)
* IRQ 1/60 second is working (e.g. TI and TI$)