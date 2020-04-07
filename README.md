# c-simple-emu-cbm #

Here is a simple Commodore 64 and 6502 Emulator I wrote from scratch, ported to C from my C# [simple-emu-c64](https://github.com/davervw/simple-emu-c64) project.  Runs in a text console window.  Optionally can specify a binary Commodore program to load, BASIC programs will auto-run.   Requires Commodore 64 ROMs (not included) present in the current directory (kernal and basic).   More details may be relevant from the [simple-emu-c64](https://github.com/davervw/simple-emu-c64) project.

Only CHRIN/CHROUT/READY(for startup program) are hooked, so no, it won't run your games, only maybe simple ASCII text adventures or such.  Sorry no PETSCII either.

Usage:

    c-simple-emu-cbm
    c-simple-emu-cbm circle.prg

![circle.bas](https://github.com/davervw/c-simple-emu-cbm/raw/master/circle.png)