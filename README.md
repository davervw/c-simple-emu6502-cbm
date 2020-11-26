# Teensy C64 #

Here is a simple Commodore 64 and 6502 Emulator I wrote from scratch, ported to C from my C# [simple-emu-c64](https://github.com/davervw/simple-emu-c64) project, and portions ported from my [STMF429Discovery](https://techwithdave.davevw.com/2020/04/commodore-64-for-stm32f429-discovery.html) project.  Requires USB host port for USB keyboard.

* Development environment is Arduino IDE (1.8.5 or later)
* Third party libraries used are Teensyduino, USBHost, and [ILI9341](https://github.com/KurtE/ILI9341_t3n.git)  
* Tested with Teensy 4.1, ILI9341 LCD screen, and USB keyboard support
* Displays C64 color text screen to LCD with PETSCII supporting two character sets (default without lowercase)
* IRQ 1/60 second is working (e.g. TI and TI$)
* Sorry emulation is limited.  No games!  No load/save to disk yet.

![](teensy41_lcd.jpg)