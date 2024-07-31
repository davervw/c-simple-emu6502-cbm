"use strict";
// c64-kbd.ts - Web browser keyboard events to Commodore 64 scan codes
//
////////////////////////////////////////////////////////////////////////////////
//
// ts-emu-c64
// C64/6502 Emulator for Web Browser
//
// MIT License
//
// Copyright (c) 2020 by David R. Van Wagner
// davevw.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
// PC(US) keyboard to Commodore keyboard symbolic mapping
// STOP(ESC) F1 F2 F3 F4 F5 F6 F7 F8 Help(F9)                  Run/Stop(Pause/Break)
//           1! 2@ 3# 4$ 5% 6^ 7& 8* 9( 0) -_ += DelIns Ins HmClr Rstr     / * -
// Ctrl(Tab) Q  W  E  R  T  Y  U  I  O  P  [  ]  £ (\)  Del       (PUp)  7 8 9 +
//           A  S  D  F  G  H  J  K  L  ;: '" Return                     4 5 6
// LShift    Z  X  C  V  B  N  M  ,< .> /?  RShift            Up         1 2 3
// C=(Ctrl)           SPACEBAR              C=(Ctrl)    Lft  Down  Rt    0 .   Enter
// Commodore 64/128 scan code table - adapted from Commodore 128 Programmer's Reference Guide by Commodore
//
//    0*8 1*8 2*8 3*8 4*8 5*8 6*8 7*8 K0  K1  K2
// +0 DEL 3#  5%  7'  9)  +   £|  1!  Hlp ESC Alt
// +1 RET W   R   Y   I   P   *   LAr 8   +   0
// +2 Rt  A   D   G   J   L   ;]  Ctl 5   -   .
// +3 F7  4$  6&  8(  0   -   Hme 2"  Tab Lf  Up
// +4 F1  Z   C   B   M   .>  RSh Spc 2   Ent Down
// +5 F3  S   F   H   K   :[  =   C=  4   6   Lt
// +6 F5  E   T   U   O   @   UpA Q   7   9   Rt
// +7 Dn  LSh X   V   N   ,<  /?  Stp 1   3   NoScroll
//
// NMI RESTORE
// 40/80 Disp
// P6510 Caps
// STA $DC00 (columns)
// LDA $DC01 (rows)
// LDA $D02F (K0-K2)
// D501 ()
//
// Commodore 64 keyboard layout (note some of the punctuation placement, matters for adapting PC keyboard to C64)
// (Normal) ←1234567890+-£ Hm Dl
//   Control qwertyuiop@*↑ Restr
// Stop ShLk asdfghjkl:;= Return
//   C= Shft zxcvbnm,./ Sh Dn Rt
//
// (Shifted) !"#$%&'()     Cl In
//           QWERTYUIOP  pi(greek letter for 3.1415..)
//           ASDFGHJKL[]=
//           ZXCVBNM<>?    Up Lt
let keyDictionary = {
    'a': { scan: 10 },
    'b': { scan: 28 },
    'c': { scan: 20 },
    'd': { scan: 18 },
    'e': { scan: 14 },
    'f': { scan: 21 },
    'g': { scan: 26 },
    'h': { scan: 29 },
    'i': { scan: 33 },
    'j': { scan: 34 },
    'k': { scan: 37 },
    'l': { scan: 42 },
    'm': { scan: 36 },
    'n': { scan: 39 },
    'o': { scan: 38 },
    'p': { scan: 41 },
    'q': { scan: 62 },
    'r': { scan: 17 },
    's': { scan: 13 },
    't': { scan: 22 },
    'u': { scan: 30 },
    'v': { scan: 31 },
    'w': { scan: 9 },
    'x': { scan: 23 },
    'y': { scan: 25 },
    'z': { scan: 12 },
    'A': { scan: 10 },
    'B': { scan: 28 },
    'C': { scan: 20 },
    'D': { scan: 18 },
    'E': { scan: 14 },
    'F': { scan: 21 },
    'G': { scan: 26 },
    'H': { scan: 29 },
    'I': { scan: 33 },
    'J': { scan: 34 },
    'K': { scan: 37 },
    'L': { scan: 42 },
    'M': { scan: 36 },
    'N': { scan: 39 },
    'O': { scan: 38 },
    'P': { scan: 41 },
    'Q': { scan: 62 },
    'R': { scan: 17 },
    'S': { scan: 13 },
    'T': { scan: 22 },
    'U': { scan: 30 },
    'V': { scan: 31 },
    'W': { scan: 9 },
    'X': { scan: 23 },
    'Y': { scan: 25 },
    'Z': { scan: 12 },
    'Enter': { scan: 1 },
    'Tab': { scan: 58 },
    'Escape': { scan: 63 },
    'Pause': { scan: 63 },
    'ShiftLeft': { scan: 15 },
    'ShiftRight': { scan: 52 },
    'ControlLeft': { scan: 61 },
    'ControlRight': { scan: 61 },
    'AltLeft': { scan: 61 },
    'AltRight': { scan: 61 },
    'Backspace': { scan: 0 },
    'Insert': { scan: 0, shift: 1 },
    'Delete': { scan: 0 },
    'Home': { scan: 51 },
    'ArrowUp': { scan: 7, shift: 1 },
    'ArrowDown': { scan: 7 },
    'ArrowLeft': { scan: 2, shift: 1 },
    'ArrowRight': { scan: 2 },
    'PageUp': { scan: 1024 },
    '1': { scan: 56, shift: 0 },
    '2': { scan: 59, shift: 0, release: '@' },
    '3': { scan: 8, shift: 0 },
    '4': { scan: 11, shift: 0 },
    '5': { scan: 16, shift: 0 },
    '6': { scan: 19, shift: 0, release: '^' },
    '7': { scan: 24, shift: 0, release: '&' },
    '8': { scan: 27, shift: 0, release: '*' },
    '9': { scan: 32, shift: 0, release: '(' },
    '0': { scan: 35, shift: 0, release: ')' },
    ' ': { scan: 60 },
    '!': { scan: 56, shift: 1 },
    '"': { scan: 59, shift: 1 },
    '#': { scan: 8, shift: 1 },
    '$': { scan: 11, shift: 1 },
    '%': { scan: 16, shift: 1 },
    '&': { scan: 19, shift: 1 },
    "'": { scan: 24, shift: 1, release: '"' },
    '(': { scan: 27, shift: 1 },
    ')': { scan: 32, shift: 1 },
    '*': { scan: 49, shift: 0 },
    '^': { scan: 54, shift: 0 },
    '@': { scan: 46, shift: 0 },
    '+': { scan: 40, shift: 0 },
    '=': { scan: 53, shift: 0, release: '+' },
    '-': { scan: 43, shift: 0, release: '_' },
    '_': { scan: 57, shift: 0 },
    ':': { scan: 45, shift: 0 },
    '[': { scan: 45, shift: 1 },
    ';': { scan: 50, shift: 0, release: ':' },
    ']': { scan: 50, shift: 1 },
    ',': { scan: 47, shift: 0 },
    '<': { scan: 47, shift: 1 },
    '.': { scan: 44, shift: 0 },
    '>': { scan: 44, shift: 1 },
    '/': { scan: 55, shift: 0 },
    '?': { scan: 55, shift: 1 },
    '\\': { scan: 48 },
    'F1': { scan: 4 },
    'F2': { scan: 4, shift: 1 },
    'F3': { scan: 5 },
    'F4': { scan: 5, shift: 1 },
    'F5': { scan: 6 },
    'F6': { scan: 6, shift: 1 },
    'F7': { scan: 3 },
    'F8': { scan: 3, shift: 1 },
    '{': { scan: 62, shift: 0, commodore: 1 },
    '}': { scan: 9, shift: 0, commodore: 1 },
    '~': { scan: 14, shift: 0, commodore: 1 },
    '`': { scan: 17, shift: 0, commodore: 1 },
    '|': { scan: 43, shift: 0, commodore: 1 },
    'Clear': { scan: 53, shift: 0 },
};
let keys = [];
let last_keys = "";
let port = null;
class C64keymapper {
    constructor() {
        var _a;
        document.addEventListener("keydown", C64keyEvent, true);
        document.addEventListener("keyup", C64keyEvent, true);
        document.addEventListener("input", C64inputEvent);
        (_a = document.getElementById("return")) === null || _a === void 0 ? void 0 : _a.addEventListener("click", C64ReturnClicked);
    }
}
function GetSerialPort() {
    return __awaiter(this, void 0, void 0, function* () {
        port = yield navigator.serial.requestPort();
        yield port.open({ baudRate: 115200 });
    });
}
function SerialWrite(msg) {
    return __awaiter(this, void 0, void 0, function* () {
        if (port == null || msg == null || msg.length == 0)
            return;
        const writer = port.writable.getWriter();
        const data = new Uint8Array(msg.length + 1);
        for (let i = 0; i < msg.length; ++i)
            data[i] = msg.charCodeAt(i);
        data[msg.length] = '|'.charCodeAt(0);
        yield writer.write(data);
        writer.releaseLock();
        console.log(msg);
    });
}
function C64ReturnClicked(ev) {
    let input = document.getElementById("input");
    let data = input.value;
    input.value = "";
    // send each key to keydown/keyup handler
    for (let i = 0; data != null && i < data.length; ++i) {
        let c = data[i];
        if (c == '\n')
            c = 'Enter';
        let evt = new KeyboardEvent("keydown", { key: c });
        C64keyEventEx(evt);
        evt = new KeyboardEvent("keyup", { key: c });
        C64keyEventEx(evt);
    }
    let c = "Enter";
    let evt = new KeyboardEvent("keydown", { key: c });
    C64keyEventEx(evt);
    evt = new KeyboardEvent("keyup", { key: c });
    C64keyEventEx(evt);
}
function C64inputEvent(ev) {
    let input = ev;
    let data = input.data;
    // // log it
    // let date = new Date();
    // let msg = (date.getSeconds()+date.getMilliseconds()/1000) + 
    //    " input data=" + input.data +
    //    //" xfer=" + input.dataTransfer +
    //    " type=" + input.inputType +
    //    " comp=" + input.isComposing;
    // const log: HTMLElement | null = document.getElementById("result");
    // if (log)
    //     log.innerHTML = log.innerHTML + "<br>" + msg;
    //console.log("input " + data);
    // remove whatever is typed from input field
    let clear = document.getElementById("clear input").checked;
    if (clear) {
        document.getElementById("input").value = "";
        // send each key to keydown/keyup handler
        let i;
        for (i = 0; !input.isComposing && data != null && i < data.length; ++i) {
            let c = data[i];
            if (c == '\n')
                c = 'Enter';
            let evt = new KeyboardEvent("keydown", { key: c });
            C64keyEventEx(evt);
            evt = new KeyboardEvent("keyup", { key: c });
            C64keyEventEx(evt);
        }
    }
}
function C64keyEvent(event) {
    let result = C64keyEventEx(event);
    if (!result) {
        event.preventDefault(); // disable all keys default actions (as allowed by OS and user agent)
        event.stopPropagation();
    }
    return result;
}
function C64keyEventEx(event) {
    let i;
    let scan = 88;
    //console.log(`key=${event.key}`);
    let key = keyDictionary[event.key];
    if (key == null)
        key = keyDictionary[event.code];
    if (key != null)
        scan = key.scan;
    let release = key === null || key === void 0 ? void 0 : key.release;
    switch (key === null || key === void 0 ? void 0 : key.shift) {
        case 0: // delete shift
            i = keys.indexOf(keyDictionary['ShiftLeft'].scan);
            if (i >= 0)
                keys.splice(i, 1);
            i = keys.indexOf(keyDictionary['ShiftRight'].scan);
            if (i >= 0)
                keys.splice(i, 1);
            break;
        case 1: // add shift
            if (keys.indexOf(keyDictionary['ShiftLeft'].scan) < 0
                && keys.indexOf(keyDictionary['ShiftRight'].scan) < 0)
                keys.push(keyDictionary['ShiftLeft'].scan);
            break;
    }
    if ((key === null || key === void 0 ? void 0 : key.commodore) === 1) {
        if (keys.indexOf(keyDictionary['AltLeft'].scan) < 0)
            keys.push(keyDictionary['AltLeft'].scan);
    }
    // log it
    // let date = new Date();
    // let modifiers = (event.metaKey ? '1' : '0') + (event.altKey ? '1' : '0') + (event.ctrlKey ? '1' : '0') + (event.shiftKey ? '1' : '0');
    // let msg = (date.getSeconds()+date.getMilliseconds()/1000) + 
    //   " " + event.type + " keyCode=" + event.keyCode + " code=" + event.code + " key=" + event.key + " modifiers=" + modifiers + " scan=" + scan;
    // //console.log(msg);
    // const log: HTMLElement | null = document.getElementById("result");
    // if (log)
    //   log.innerHTML = log.innerHTML + "<br>" + msg;
    //console.log(event.type + " " + event.key + " " + event.code + " " + scan);
    if (event.type == "keydown") {
        if (scan != 88) {
            if (keys.indexOf(scan) < 0)
                keys.push(scan);
        }
    }
    else if (event.type == "keyup") {
        i = keys.indexOf(scan);
        if (i < 0 && !event.shiftKey && release != null) // did we get keyup for unshifted version of keydown and keyCode did not match
            i = keys.indexOf(keyDictionary[release].scan);
        if (i >= 0)
            keys.splice(i, 1);
        if (keys.length > 0 && !event.shiftKey) {
            // browser bug or feature: pressing both shift keys, releasing both only gets one release
            // if no shift keys reported, make sure we lift keys from our array
            i = keys.indexOf(52); // Right Shift
            if (i >= 0)
                keys.splice(i, 1);
            i = keys.indexOf(15); // Left Shift
            if (i >= 0)
                keys.splice(i, 1);
        }
        if (keys.length > 0 && !event.altKey && !event.ctrlKey) {
            // browser may miss reporting keyup on Alt, e.g. toggle menus, ALT+Tab
            i = keys.indexOf(61); // C=
            if (i >= 0)
                keys.splice(i, 1);
        }
    }
    let msg = keys.toString();
    if (msg.length == 0)
        msg = '88';
    if (msg != last_keys) {
        SerialWrite(msg);
        last_keys = msg;
    }
    return (scan == 88);
}
const mapper = new C64keymapper();
