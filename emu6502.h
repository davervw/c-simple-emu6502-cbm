// emu6502.h - Emu6502 - MOS6502 Emulator
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C Portable Version)
// C64/6502 Emulator for Microsoft Windows Console
//
// MIT License
//
// Copyright(c) 2020 by David R.Van Wagner
// davevw.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

#pragma once

typedef signed char sbyte;
typedef unsigned char byte;
typedef unsigned char bool;
typedef unsigned short ushort;
#define false 0
#define true 1

extern byte A;
extern byte X;
extern byte Y;
extern byte S;
extern bool N;
extern bool V;
extern bool B;
extern bool D;
extern bool I;
extern bool Z;
extern bool C;
extern ushort PC;

extern bool trace;
extern bool step;

extern void ResetRun(bool (*ExecutePatch)(void));
extern void Execute(ushort addr, bool (*ExecutePatch)(void));
extern void Push(int value);
extern byte Pop(void);
extern void DisassembleLong(ushort addr, bool *p_conditional, byte *p_bytes, ushort *p_addr2, char *dis, int dis_size, char *line, int line_size);
extern void DisassembleShort(ushort addr, bool *p_conditional, byte *p_bytes, ushort *p_addr2, char *dis, int dis_size);
extern byte LO(ushort value);
extern byte HI(ushort value);

extern void SetMemory(ushort addr, byte value);
extern byte GetMemory(ushort addr);
extern bool ExecutePatch(void);
