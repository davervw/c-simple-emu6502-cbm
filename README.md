# c-simple-emu-cbm #

Here is a simple Commodore 64/128 and 6502 Emulator I wrote from scratch, ported to C++ from my C# [simple-emu-c64](https://github.com/davervw/simple-emu-c64) project.  Runs in a text console window.  Optionally can specify a binary Commodore program to load, BASIC programs will auto-run.   Requires Commodore ROMs (not included) present in the roms folder.   More details may be relevant from the [simple-emu-c64](https://github.com/davervw/simple-emu-c64) project.

>
> TLDR; This is the terminal version, fast-forward to [Unified](https://github.com/davervw/c-simple-emu6502-cbm/blob/master/README.md#unified-lcd--graphical) to see a graphical version
>

```
roms/c128/basichi
roms/c128/basiclo
roms/c128/chargen
roms/c128/kernal
roms/c64/basic
roms/c64/chargen
roms/c64/kernal
roms/pet/basic1
roms/pet/characters.bin
roms/pet/characters2.bin
roms/pet/edit1g
roms/pet/kernal1
roms/ted/basic
roms/ted/kernal
roms/test/6502test.bin
roms/vic20/basic
roms/vic20/chargen
roms/vic20/kernal
```

## C++ Version ##

This is a mostly object-oriented version to more closely match the C# version now including PET model 2001, Vic-20, Commodore 64, Commodore 128, Commodore 16, Commodore Plus/4.   But goodbye plain old C; we will miss you.  Long live C++.  (The original port from C# to C is still available in the commit history.)

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

This portable version (master branch unless otherwise specified) has been tested with:

### Console terminal versions ###

* Microsoft Windows 11 (Visual Studio 2022) x64
* cygwin (gcc) x64
* Ubuntu Linux (gcc) x64
* Raspberry Pi 4 (gcc, Linux ARM)
* Rock Pi S (Rockchip RK3308, gcc, Linux ARM)
* NXP LPC1768 (ARM MBED): [forked](https://os.mbed.com/users/davervw/code/c-simple-emu6502-cbm/)
* [Arduino branch](https://github.com/davervw/c-simple-emu6502-cbm/tree/arduino) from master

### LCD versions ###

* STM32F429 LCD (ARM MBED, note must rename ADC() method) [forked](https://os.mbed.com/users/davervw/code/C64-stm429_discovery/)
* [M5_core branch](https://github.com/davervw/c-simple-emu6502-cbm/tree/m5_core) original M5 Core support (no PSRAM RAM)
* [m5-atom-s3 branch](https://github.com/davervw/c-simple-emu6502-cbm/tree/m5_atom_s3) for M5 Atom S3
* [m5stick-c branch](https://github.com/davervw/c-simple-emu6502-cbm/tree/m5stickc) for M5Stick-C
* [m5-stamp-s3 branch](https://github.com/davervw/c-simple-emu6502-cbm/tree/m5_stamp_s3) for M5 Stamp-S3

### [Unified](https://github.com/davervw/c-simple-emu6502-cbm/tree/unified) (LCD / Graphical) ###

The following branches and targets have been consolidated into a single [Unified](https://github.com/davervw/c-simple-emu6502-cbm/tree/unified) branch for improved sharing of code and features.  It is recommended to follow the unified branch instead of individual branches.

* Teensy_LCD branch from Arduino, keyboard mapping from STM32
* M5 branch branched from Teensy_LCD for M5FireIoT, M5Core2, M5CoreS3, with merges from cpp branch for maximum features
* lilygo-t-display-s3 branch for LilyGo T-Display-S3
* ESP32-8048S070-7inch branch for Sunton ESP32-8048S070
* Microsoft Windows graphical version for desktops, laptops, tablets (requires Direct2D)

### Limitations ###

* Terminal console versions only hook the CHRIN/CHROUT/READY(for startup program)/SETLFS/SETNAM/LOAD/SAVE kernal routines, so no, it won't run your favorite games, only maybe simple ASCII text adventures or such.  Sorry no PETSCII either (except LCD versions, or if you [add PETSCII font to Windows](https://style64.org/c64-truetype)).   
* Some versions (Windows, Linux, Teensy, M5, etc.) with lots of RAM available also have D64 support for simple emulation of 1541 disk images so you can have a persistant collection of files that should be compatible with other emulators.

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
