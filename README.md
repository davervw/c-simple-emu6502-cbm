# c-simple-emu-cbm #

Here is a simple Commodore 64/128 and 6502 Emulator I wrote from scratch, ported to C++ from my C# [simple-emu-c64](https://github.com/davervw/simple-emu-c64) project.  Runs in a text console window.  Optionally can specify a binary Commodore program to load, BASIC programs will auto-run.   Requires Commodore 64 ROMs (not included) present in the current directory (chargen, kernal and basic).   More details may be relevant from the [simple-emu-c64](https://github.com/davervw/simple-emu-c64) project.

## C++ Version ##

This is a mostly object-oriented version to more closely match the C# version now including PET model 2001, Vic-20, Commodore 64, Commodore 128, Commodore 16, Commodore Plus/4.   But goodbye plain old C; we will miss you.  Long live C++.  (The original port from C# to C is still available in the branches, and commit history.)

## GO 128 ##

Special feature is called GO 128.  Just like the counterpart GO 64 implemented in the C128 ROMs, GO 128 will reset the system from 64 to 128 mode.   And GO 64 and GO 128 both work in their respective environments to completely reset them, not retaining any memory or state (except disk state).  Switching to other systems works too!  

Note that unlike the real C128, in this emulator GO 64 will not retain any memory between the environments, nor does the C64 environment share any extras from the C128 system.

````
GO 2001
GO 20
GO 64
GO 128
GO 16
GO 4
````

## Cross platform ##

This portable version has been tested with:

* Microsoft Windows 10 (Visual Studio 2017) x64
* Ubuntu Linux (g++) x64

Only CHRIN/CHROUT/READY(for startup program)/SETLFS/SETNAM/LOAD/SAVE are hooked, so no, it won't run your favorite games, only maybe simple ASCII text adventures or such.  Sorry no PETSCII either (see LCD versions).   Some versions (Windows, Linux, Teensy, M5) also have D64 support for simple emulation of 1541 disk images so you can have a persistant collection of files that should be compatible with other emulators.

## Usage ##

    c-simple-emu-cbm
    c-simple-emu-cbm samples.d64
    c-simple-emu-cbm hello.prg

If a .prg is loaded from the command line, it will create a new disk with the same name and extension .d64 unless it already exists.

![circle.bas](https://github.com/davervw/c-simple-emu6502-cbm/raw/master/circle.png)

## Credits ##

Thanks to [Lars Gregori](https://github.com/choas) for clock changes, using system time as available on Windows, Linux, etc. (C64 only for now)

gettimeofday.c is also under MIT License:
 * Copyright (c) 2003 SRA, Inc.
 * Copyright (c) 2003 SKC, Inc.
