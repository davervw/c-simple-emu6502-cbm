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
var __generator = (this && this.__generator) || function (thisArg, body) {
    var _ = { label: 0, sent: function() { if (t[0] & 1) throw t[1]; return t[1]; }, trys: [], ops: [] }, f, y, t, g;
    return g = { next: verb(0), "throw": verb(1), "return": verb(2) }, typeof Symbol === "function" && (g[Symbol.iterator] = function() { return this; }), g;
    function verb(n) { return function (v) { return step([n, v]); }; }
    function step(op) {
        if (f) throw new TypeError("Generator is already executing.");
        while (g && (g = 0, op[0] && (_ = 0)), _) try {
            if (f = 1, y && (t = op[0] & 2 ? y["return"] : op[0] ? y["throw"] || ((t = y["return"]) && t.call(y), 0) : y.next) && !(t = t.call(y, op[1])).done) return t;
            if (y = 0, t) op = [op[0] & 2, t.value];
            switch (op[0]) {
                case 0: case 1: t = op; break;
                case 4: _.label++; return { value: op[1], done: false };
                case 5: _.label++; y = op[1]; op = [0]; continue;
                case 7: op = _.ops.pop(); _.trys.pop(); continue;
                default:
                    if (!(t = _.trys, t = t.length > 0 && t[t.length - 1]) && (op[0] === 6 || op[0] === 2)) { _ = 0; continue; }
                    if (op[0] === 3 && (!t || (op[1] > t[0] && op[1] < t[3]))) { _.label = op[1]; break; }
                    if (op[0] === 6 && _.label < t[1]) { _.label = t[1]; t = op; break; }
                    if (t && _.label < t[2]) { _.label = t[2]; _.ops.push(op); break; }
                    if (t[2]) _.ops.pop();
                    _.trys.pop(); continue;
            }
            op = body.call(thisArg, _);
        } catch (e) { op = [6, e]; y = 0; } finally { f = t = 0; }
        if (op[0] & 5) throw op[1]; return { value: op[0] ? op[1] : void 0, done: true };
    }
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
var keyDictionary = {
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
    'PageUp': { scan: 1024 + 64 },
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
    'F8': { scan: 3, shift: 1 }
};
var keys = [];
var last_keys = "";
var port = null;
var C64keymapper = /** @class */ (function () {
    function C64keymapper() {
        var _a;
        document.addEventListener("keydown", C64keyEvent, true);
        document.addEventListener("keyup", C64keyEvent, true);
        document.addEventListener("input", C64inputEvent);
        (_a = document.getElementById("return")) === null || _a === void 0 ? void 0 : _a.addEventListener("click", C64ReturnClicked);
    }
    return C64keymapper;
}());
function GetSerialPort() {
    return __awaiter(this, void 0, void 0, function () {
        return __generator(this, function (_a) {
            switch (_a.label) {
                case 0: return [4 /*yield*/, navigator.serial.requestPort()];
                case 1:
                    port = _a.sent();
                    return [4 /*yield*/, port.open({ baudRate: 115200 })];
                case 2:
                    _a.sent();
                    return [2 /*return*/];
            }
        });
    });
}
function SerialWrite(msg) {
    return __awaiter(this, void 0, void 0, function () {
        var writer, data, i;
        return __generator(this, function (_a) {
            switch (_a.label) {
                case 0:
                    if (port == null || msg == null || msg.length == 0)
                        return [2 /*return*/];
                    writer = port.writable.getWriter();
                    data = new Uint8Array(msg.length + 1);
                    for (i = 0; i < msg.length; ++i)
                        data[i] = msg.charCodeAt(i);
                    data[msg.length] = '|'.charCodeAt(0);
                    return [4 /*yield*/, writer.write(data)];
                case 1:
                    _a.sent();
                    writer.releaseLock();
                    console.log(msg);
                    return [2 /*return*/];
            }
        });
    });
}
function C64ReturnClicked(ev) {
    var input = document.getElementById("input");
    var data = input.value;
    input.value = "";
    // send each key to keydown/keyup handler
    for (var i = 0; data != null && i < data.length; ++i) {
        var c_1 = data[i];
        if (c_1 == '\n')
            c_1 = 'Enter';
        var evt_1 = new KeyboardEvent("keydown", { key: c_1 });
        C64keyEventEx(evt_1);
        evt_1 = new KeyboardEvent("keyup", { key: c_1 });
        C64keyEventEx(evt_1);
    }
    var c = "Enter";
    var evt = new KeyboardEvent("keydown", { key: c });
    C64keyEventEx(evt);
    evt = new KeyboardEvent("keyup", { key: c });
    C64keyEventEx(evt);
}
function C64inputEvent(ev) {
    var input = ev;
    var data = input.data;
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
    var clear = document.getElementById("clear input").checked;
    if (clear) {
        document.getElementById("input").value = "";
        // send each key to keydown/keyup handler
        var i = void 0;
        for (i = 0; !input.isComposing && data != null && i < data.length; ++i) {
            var c = data[i];
            if (c == '\n')
                c = 'Enter';
            var evt = new KeyboardEvent("keydown", { key: c });
            C64keyEventEx(evt);
            evt = new KeyboardEvent("keyup", { key: c });
            C64keyEventEx(evt);
        }
    }
}
function C64keyEvent(event) {
    var result = C64keyEventEx(event);
    if (!result) {
        event.preventDefault(); // disable all keys default actions (as allowed by OS and user agent)
        event.stopPropagation();
    }
    return result;
}
function C64keyEventEx(event) {
    var i;
    var scan = 64;
    var key = keyDictionary[event.key];
    if (key == null)
        key = keyDictionary[event.code];
    if (key != null)
        scan = key.scan;
    var release = key === null || key === void 0 ? void 0 : key.release;
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
        if (scan != 64) {
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
    var msg = keys.toString();
    if (msg.length == 0)
        msg = '64';
    if (msg != last_keys) {
        SerialWrite(msg);
        last_keys = msg;
    }
    return (scan == 64);
}
var mapper = new C64keymapper();
//# sourceMappingURL=c64-main-ui.js.map