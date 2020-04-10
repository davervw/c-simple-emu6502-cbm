# c-simple-emu-cbm #

Here is a simple Commodore 64 and 6502 Emulator I wrote from scratch, ported to C from my C# [simple-emu-c64](https://github.com/davervw/simple-emu-c64) project.  Runs in a text console window.  Optionally can specify a binary Commodore program to load, BASIC programs will auto-run.   Requires Commodore 64 ROMs (not included) present in the current directory (kernal and basic).   More details may be relevant from the [simple-emu-c64](https://github.com/davervw/simple-emu-c64) project.

This portable version has been tested with:

* Microsoft Windows 10 (Visual Studio 2017) x64
* cygwin (gcc) x64
* Ubuntu Linux (gcc) x64
* Raspberry Pi 4 (gcc, Linux ARM)
* NXP LPC1768 (ARM MBED): [forked](https://os.mbed.com/users/davervw/code/c-simple-emu6502-cbm/)
* STM32F429 (ARM MBED, note must rename ADC() method)
* [Arduino](https://github.com/davervw/c-simple-emu6502-cbm/tree/arduino) branched from [master](https://github.com/davervw/c-simple-emu6502-cbm/tree/master)

Only CHRIN/CHROUT/READY(for startup program) are hooked, so no, it won't run your games, only maybe simple ASCII text adventures or such.  Sorry no PETSCII either.

Usage:

    c-simple-emu-cbm
    c-simple-emu-cbm circle.prg
    c-simple-emu-cbm guess1.prg
    c-simple-emu-cbm guess2.prg

![circle.bas](https://github.com/davervw/c-simple-emu6502-cbm/raw/master/circle.png)
