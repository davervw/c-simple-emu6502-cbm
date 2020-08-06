# c-simple-emu-cbm #

Here is a simple Commodore 64 and 6502 Emulator I wrote from scratch, ported to C from my C# [simple-emu-c64](https://github.com/davervw/simple-emu-c64) project.  Runs in a text console window.  Optionally can specify a binary Commodore program to load, BASIC programs will auto-run.   Requires Commodore 64 ROMs (not included) present in the current directory (kernal and basic).   More details may be relevant from the [simple-emu-c64](https://github.com/davervw/simple-emu-c64) project.

The portable version has been tested with multiple platforms.

This [Arduino](https://github.com/davervw/c-simple-emu6502-cbm/tree/arduino) version is branched from [master](https://github.com/davervw/c-simple-emu6502-cbm/tree/master) and tested specifically with:

* Arduino Due (ARM ATSAM3X8E)
* Teensy 3.5
* Teensy 4.1

Only CHRIN/CHROUT are hooked, so no, it won't run your games, only maybe simple ASCII text adventures or such.  Sorry no PETSCII either.

![circle.bas](https://github.com/davervw/c-simple-emu6502-cbm/raw/arduino/circle.png)
