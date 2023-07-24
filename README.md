# c-simple-emu-cbm #

Here is a simple Commodore 64 and 6502 Emulator I wrote from scratch, ported to C (and some C++) from my C# [simple-emu-c64](https://github.com/davervw/simple-emu-c64) project.  Runs in a text console window.  Optionally can specify a binary Commodore program to load, BASIC programs will auto-run.   Requires Commodore 64 ROMs (not included) present in the current directory (chargen, kernal and basic).   More details may be relevant from the [simple-emu-c64](https://github.com/davervw/simple-emu-c64) project.

This portable version has been tested with:

* Microsoft Windows 10 (Visual Studio 2017) x64
* cygwin (gcc) x64
* Ubuntu Linux (gcc) x64
* Raspberry Pi 4 (gcc, Linux ARM)
* NXP LPC1768 (ARM MBED): [forked](https://os.mbed.com/users/davervw/code/c-simple-emu6502-cbm/)
* STM32F429 LCD (ARM MBED, note must rename ADC() method) [forked](https://os.mbed.com/users/davervw/code/C64-stm429_discovery/)
* [Arduino](https://github.com/davervw/c-simple-emu6502-cbm/tree/arduino) branched from [master](https://github.com/davervw/c-simple-emu6502-cbm/tree/master)
* [Teensy_LCD](https://github.com/davervw/c-simple-emu6502-cbm/tree/teensy_lcd) branched from Arduino, keyboard mapping from STM32
* [M5](https://github.com/davervw/c-simple-emu6502-cbm/tree/m5) branched from Teensy_LCD for M5Core2, M5CoreS3

Only CHRIN/CHROUT/READY(for startup program)/SETLFS/SETNAM/LOAD/SAVE are hooked, so no, it won't run your favorite games, only maybe simple ASCII text adventures or such.  Sorry no PETSCII either (except LCD versions).   Some versions (Windows, Linux, Teensy, M5) also have D64 support for simple emulation of 1541 disk images so you can have a persistant collection of files that should be compatible with other emulators.

Usage:

    c-simple-emu-cbm
    c-simple-emu-cbm samples.d64
    c-simple-emu-cbm hello.prg

If a .prg is loaded from the command line, it will create a new disk with the same name and extension .d64 unless it already exists.

![circle.bas](https://github.com/davervw/c-simple-emu6502-cbm/raw/master/circle.png)

Credits:

Thanks to [Lars Gregori](https://github.com/choas) for clock changes, using system time as available on Windows, Linux, etc.

gettimeofday.c is also under MIT License:
 * Copyright (c) 2003 SRA, Inc.
 * Copyright (c) 2003 SKC, Inc.
