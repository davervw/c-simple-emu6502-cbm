# LilyGo T-Display-S3 #

Another simple C64 emulator port for [LilyGo T-Display-S3](https://www.lilygo.cc/products/t-display-s3)

![](t-display-s3.jpg)

Supports browser-keyscan-helper (see included folder) for connecting via USB Serial to PC, use Chrome browser for keyboard entry

Here are the full steps to get the project going, including enabling typing from Chrome browser.

1. Clone [https://github.com/davervw/c-simple-emu6502-cbm](https://github.com/davervw/c-simple-emu6502-cbm)
2. Checkout branch lilygo-t-display-s3
3. Launch Arduino IDE 2.x.  I am using Arduino IDE 2.1.1
4. Select device LilyGo T-Display-S3 and COM port appropriately
5. Requires library dependency: [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) Instructions: [Volos Projects](https://www.youtube.com/watch?v=WFVjsxFMbSM)
6. Configure USB CDC on Boot to Enabled so that Serial is routed to the USB-C port
7. Click Upload to build and flash to the ESP32.   Make sure #5 setting USB CDC is still enabled, otherwise repeat from #6.
8. Shutdown Arduino 2.x to make sure no one is actively connected to USB serial port
9. Click reset on T-Display-S3 for good measure to make sure no one is actively connected to USB serial port
10. Browse to browser-keyscan-helper folder in project
11. Launch index.html to start keyboard helper (preferably Google Chrome, haven't tried others)
12. Click Connect button
13. Select corresponding COM port to ESP32 and click Connect
14. Keep focus on the keyboard helper web page, and keystrokes should be sent over USB
15. If having trouble, check step #6 again
16. LOAD/SAVE are supported for drives 8 and 9 (and drive 8 is assumed if no number is specified).  [Format](https://github.com/davervw/filecmdproc-esp32) a FATFS partition in flash storage with /disk/drive8.d64 and drive9.d64 [uploaded](https://github.com/smford/esp32-asyncwebserver-fileupload-example).

## Keyboard hints ##
```
CTRL and ALT are both mapped to Commodore Logo C= key
TAB is mapped to Commodore CTRL
TAB+1..8 are colors
TAB+9 Commodore Reverse On
TAB+0 Commodore Reverse Off
ALT+1..8 are more colors
ESC is Commodore STOP
PgUp is Commodore RESTORE
ESC+PgUp is STOP+RESTORE for NMI reset of system
SHIFT+HOME is Commodore Clear Screen
CTRL+SHIFT or ALT+SHIFT to toggle Uppercase/Graphics and Lowercase/Uppercase fonts
```
## Example statements ##
```
10 PRINT "HELLO ESP! ";
20 GOTO 10
RUN
(then ESC to STOP)

POKE 53281,2
(background color 0..15)

POKE 53280,3
(border color 0..15)

SYS 64738
(resets Commodore)
```
