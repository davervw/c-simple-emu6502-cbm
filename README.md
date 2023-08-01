# M5Core C64 with GO 128 #

![GO 128](media/c128_on_m5.png)

Work in progress.  Current state includes rendering full color screen on M5Core LCD and supporting multiple Core models with a define change in M5Core.h.  60 times a second IRQ implemented to blink cursor. Requires SD support for roms, tested with Core2 and CoreS3 (must change defines in local M5Core.h).   GO 128 and GO 64 switch back and forth.

Build instructions

1. Clone repository, switch to branch m5
2. Open m5cbm/m5cbm.ino with Arduino 2.x IDE
3. Modify M5Core.h to #define target, e.g. Core2 or CoreS3 (note Basic not supported at this time, see below)
4. Build and deploy to M5Stack Core device, should complain "Card Mount Failed"
5. Insert MicroSD with the following files (roms from Vice or similar, disk files optional), and reset

```
roms\c64\basic
roms\c64\chargen
roms\c64\kernal
roms\c128\basiclo
roms\c128\basichi
roms\c128\chargen
roms\c128\kernal
disks\drive8.d64
disks\drive9.d64
```

Notes:

* LOAD/SAVE/VERIFY commands are intercepted by emulator.  There are some bugs in C128 for LOAD, so user beware.
* GO 128 command added for switching to Commodore 128 mode (how? intercepted by the emulator)
* Keyboard is a serial attached helper that sends scan codes.  It can either be a web page -- see [browser-keyscan-helper](https://github.com/davervw/c-simple-emu6502-cbm/tree/m5/browser-keyscan-helper) with USB serial attachment, or a physical device attached to Port.A -- see project [c128_keyscan](https://github.com/davervw/c128_keyscan/tree/ninetyone_tx2_itsy_bitsy).  I use both standard USB or Bluetooth keyboards, and my original C128D external keyboard.

Open browser-keyscan-helper/index.html to run an adapter with instructions how to use a keyboard via serial from a desktop web browser (e.g. Chrome).

```
TODO: integrate web-serial-polyfill because Chrome mobile web browser doesn't support Serial APIs directly.
```

![M5 Basic Core shown next to mini USB keyboard](browser-keyscan-helper/core_keyboard.jpg)

![Photo showing bluetooth Palm Portable Keyboard, Phone running key scan helper with serial USB to M5](browser-keyscan-helper/palm_phone_serial.jpg)

![Block diagram showing bluetooth Palm Portable Keyboard, Phone running key scan helper with serial USB to M5](browser-keyscan-helper/block_diagram.png)

![Early prototype with various M5 Core models](media/m5cores.jpg)

* Basic Core (not supported without extra RAM)
* Core2
* CoreS3
* Whoops, screen not big enough on StickC
