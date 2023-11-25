// emu6502.cpp - Emu6502 - MOS6502 Emulator
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C Portable Version)
// C64/6502 Emulator for Microsoft Windows Console
//
// MIT License
//
// Copyright (c) 2023 by David R. Van Wagner
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef WIN32
#include <windows.h> // OutputDebugStringA
#define snprintf sprintf_s
#endif

#include "emu6502.h"

Emu6502::Emu6502(Memory* mem)
{
	memory = mem;
	A = 0;
	X = 0;
	Y = 0;
	S = 0xFF;
	N = false;
	V = false;
	B = false;
	D = false;
	I = false;
	Z = false;
	C = false;
	PC = 0;

	trace = false;
	step = false;
	quit = false;

	ExecuteOp[0x00] = &Emu6502::ExecuteOp00;
	ExecuteOp[0x01] = &Emu6502::ExecuteOp01;
	ExecuteOp[0x02] = &Emu6502::ExecuteOp02;
	ExecuteOp[0x03] = &Emu6502::ExecuteOp03;
	ExecuteOp[0x04] = &Emu6502::ExecuteOp04;
	ExecuteOp[0x05] = &Emu6502::ExecuteOp05;
	ExecuteOp[0x06] = &Emu6502::ExecuteOp06;
	ExecuteOp[0x07] = &Emu6502::ExecuteOp07;
	ExecuteOp[0x08] = &Emu6502::ExecuteOp08;
	ExecuteOp[0x09] = &Emu6502::ExecuteOp09;
	ExecuteOp[0x0A] = &Emu6502::ExecuteOp0A;
	ExecuteOp[0x0B] = &Emu6502::ExecuteOp0B;
	ExecuteOp[0x0C] = &Emu6502::ExecuteOp0C;
	ExecuteOp[0x0D] = &Emu6502::ExecuteOp0D;
	ExecuteOp[0x0E] = &Emu6502::ExecuteOp0E;
	ExecuteOp[0x0F] = &Emu6502::ExecuteOp0F;

	ExecuteOp[0x10] = &Emu6502::ExecuteOp10;
	ExecuteOp[0x11] = &Emu6502::ExecuteOp11;
	ExecuteOp[0x12] = &Emu6502::ExecuteOp12;
	ExecuteOp[0x13] = &Emu6502::ExecuteOp13;
	ExecuteOp[0x14] = &Emu6502::ExecuteOp14;
	ExecuteOp[0x15] = &Emu6502::ExecuteOp15;
	ExecuteOp[0x16] = &Emu6502::ExecuteOp16;
	ExecuteOp[0x17] = &Emu6502::ExecuteOp17;
	ExecuteOp[0x18] = &Emu6502::ExecuteOp18;
	ExecuteOp[0x19] = &Emu6502::ExecuteOp19;
	ExecuteOp[0x1A] = &Emu6502::ExecuteOp1A;
	ExecuteOp[0x1B] = &Emu6502::ExecuteOp1B;
	ExecuteOp[0x1C] = &Emu6502::ExecuteOp1C;
	ExecuteOp[0x1D] = &Emu6502::ExecuteOp1D;
	ExecuteOp[0x1E] = &Emu6502::ExecuteOp1E;
	ExecuteOp[0x1F] = &Emu6502::ExecuteOp1F;

	ExecuteOp[0x20] = &Emu6502::ExecuteOp20;
	ExecuteOp[0x21] = &Emu6502::ExecuteOp21;
	ExecuteOp[0x22] = &Emu6502::ExecuteOp22;
	ExecuteOp[0x23] = &Emu6502::ExecuteOp23;
	ExecuteOp[0x24] = &Emu6502::ExecuteOp24;
	ExecuteOp[0x25] = &Emu6502::ExecuteOp25;
	ExecuteOp[0x26] = &Emu6502::ExecuteOp26;
	ExecuteOp[0x27] = &Emu6502::ExecuteOp27;
	ExecuteOp[0x28] = &Emu6502::ExecuteOp28;
	ExecuteOp[0x29] = &Emu6502::ExecuteOp29;
	ExecuteOp[0x2A] = &Emu6502::ExecuteOp2A;
	ExecuteOp[0x2B] = &Emu6502::ExecuteOp2B;
	ExecuteOp[0x2C] = &Emu6502::ExecuteOp2C;
	ExecuteOp[0x2D] = &Emu6502::ExecuteOp2D;
	ExecuteOp[0x2E] = &Emu6502::ExecuteOp2E;
	ExecuteOp[0x2F] = &Emu6502::ExecuteOp2F;

	ExecuteOp[0x30] = &Emu6502::ExecuteOp30;
	ExecuteOp[0x31] = &Emu6502::ExecuteOp31;
	ExecuteOp[0x32] = &Emu6502::ExecuteOp32;
	ExecuteOp[0x33] = &Emu6502::ExecuteOp33;
	ExecuteOp[0x34] = &Emu6502::ExecuteOp34;
	ExecuteOp[0x35] = &Emu6502::ExecuteOp35;
	ExecuteOp[0x36] = &Emu6502::ExecuteOp36;
	ExecuteOp[0x37] = &Emu6502::ExecuteOp37;
	ExecuteOp[0x38] = &Emu6502::ExecuteOp38;
	ExecuteOp[0x39] = &Emu6502::ExecuteOp39;
	ExecuteOp[0x3A] = &Emu6502::ExecuteOp3A;
	ExecuteOp[0x3B] = &Emu6502::ExecuteOp3B;
	ExecuteOp[0x3C] = &Emu6502::ExecuteOp3C;
	ExecuteOp[0x3D] = &Emu6502::ExecuteOp3D;
	ExecuteOp[0x3E] = &Emu6502::ExecuteOp3E;
	ExecuteOp[0x3F] = &Emu6502::ExecuteOp3F;

	ExecuteOp[0x40] = &Emu6502::ExecuteOp40;
	ExecuteOp[0x41] = &Emu6502::ExecuteOp41;
	ExecuteOp[0x42] = &Emu6502::ExecuteOp42;
	ExecuteOp[0x43] = &Emu6502::ExecuteOp43;
	ExecuteOp[0x44] = &Emu6502::ExecuteOp44;
	ExecuteOp[0x45] = &Emu6502::ExecuteOp45;
	ExecuteOp[0x46] = &Emu6502::ExecuteOp46;
	ExecuteOp[0x47] = &Emu6502::ExecuteOp47;
	ExecuteOp[0x48] = &Emu6502::ExecuteOp48;
	ExecuteOp[0x49] = &Emu6502::ExecuteOp49;
	ExecuteOp[0x4A] = &Emu6502::ExecuteOp4A;
	ExecuteOp[0x4B] = &Emu6502::ExecuteOp4B;
	ExecuteOp[0x4C] = &Emu6502::ExecuteOp4C;
	ExecuteOp[0x4D] = &Emu6502::ExecuteOp4D;
	ExecuteOp[0x4E] = &Emu6502::ExecuteOp4E;
	ExecuteOp[0x4F] = &Emu6502::ExecuteOp4F;

	ExecuteOp[0x50] = &Emu6502::ExecuteOp50;
	ExecuteOp[0x51] = &Emu6502::ExecuteOp51;
	ExecuteOp[0x52] = &Emu6502::ExecuteOp52;
	ExecuteOp[0x53] = &Emu6502::ExecuteOp53;
	ExecuteOp[0x54] = &Emu6502::ExecuteOp54;
	ExecuteOp[0x55] = &Emu6502::ExecuteOp55;
	ExecuteOp[0x56] = &Emu6502::ExecuteOp56;
	ExecuteOp[0x57] = &Emu6502::ExecuteOp57;
	ExecuteOp[0x58] = &Emu6502::ExecuteOp58;
	ExecuteOp[0x59] = &Emu6502::ExecuteOp59;
	ExecuteOp[0x5A] = &Emu6502::ExecuteOp5A;
	ExecuteOp[0x5B] = &Emu6502::ExecuteOp5B;
	ExecuteOp[0x5C] = &Emu6502::ExecuteOp5C;
	ExecuteOp[0x5D] = &Emu6502::ExecuteOp5D;
	ExecuteOp[0x5E] = &Emu6502::ExecuteOp5E;
	ExecuteOp[0x5F] = &Emu6502::ExecuteOp5F;

	ExecuteOp[0x60] = &Emu6502::ExecuteOp60;
	ExecuteOp[0x61] = &Emu6502::ExecuteOp61;
	ExecuteOp[0x62] = &Emu6502::ExecuteOp62;
	ExecuteOp[0x63] = &Emu6502::ExecuteOp63;
	ExecuteOp[0x64] = &Emu6502::ExecuteOp64;
	ExecuteOp[0x65] = &Emu6502::ExecuteOp65;
	ExecuteOp[0x66] = &Emu6502::ExecuteOp66;
	ExecuteOp[0x67] = &Emu6502::ExecuteOp67;
	ExecuteOp[0x68] = &Emu6502::ExecuteOp68;
	ExecuteOp[0x69] = &Emu6502::ExecuteOp69;
	ExecuteOp[0x6A] = &Emu6502::ExecuteOp6A;
	ExecuteOp[0x6B] = &Emu6502::ExecuteOp6B;
	ExecuteOp[0x6C] = &Emu6502::ExecuteOp6C;
	ExecuteOp[0x6D] = &Emu6502::ExecuteOp6D;
	ExecuteOp[0x6E] = &Emu6502::ExecuteOp6E;
	ExecuteOp[0x6F] = &Emu6502::ExecuteOp6F;

	ExecuteOp[0x70] = &Emu6502::ExecuteOp70;
	ExecuteOp[0x71] = &Emu6502::ExecuteOp71;
	ExecuteOp[0x72] = &Emu6502::ExecuteOp72;
	ExecuteOp[0x73] = &Emu6502::ExecuteOp73;
	ExecuteOp[0x74] = &Emu6502::ExecuteOp74;
	ExecuteOp[0x75] = &Emu6502::ExecuteOp75;
	ExecuteOp[0x76] = &Emu6502::ExecuteOp76;
	ExecuteOp[0x77] = &Emu6502::ExecuteOp77;
	ExecuteOp[0x78] = &Emu6502::ExecuteOp78;
	ExecuteOp[0x79] = &Emu6502::ExecuteOp79;
	ExecuteOp[0x7A] = &Emu6502::ExecuteOp7A;
	ExecuteOp[0x7B] = &Emu6502::ExecuteOp7B;
	ExecuteOp[0x7C] = &Emu6502::ExecuteOp7C;
	ExecuteOp[0x7D] = &Emu6502::ExecuteOp7D;
	ExecuteOp[0x7E] = &Emu6502::ExecuteOp7E;
	ExecuteOp[0x7F] = &Emu6502::ExecuteOp7F;

	ExecuteOp[0x80] = &Emu6502::ExecuteOp80;
	ExecuteOp[0x81] = &Emu6502::ExecuteOp81;
	ExecuteOp[0x82] = &Emu6502::ExecuteOp82;
	ExecuteOp[0x83] = &Emu6502::ExecuteOp83;
	ExecuteOp[0x84] = &Emu6502::ExecuteOp84;
	ExecuteOp[0x85] = &Emu6502::ExecuteOp85;
	ExecuteOp[0x86] = &Emu6502::ExecuteOp86;
	ExecuteOp[0x87] = &Emu6502::ExecuteOp87;
	ExecuteOp[0x88] = &Emu6502::ExecuteOp88;
	ExecuteOp[0x89] = &Emu6502::ExecuteOp89;
	ExecuteOp[0x8A] = &Emu6502::ExecuteOp8A;
	ExecuteOp[0x8B] = &Emu6502::ExecuteOp8B;
	ExecuteOp[0x8C] = &Emu6502::ExecuteOp8C;
	ExecuteOp[0x8D] = &Emu6502::ExecuteOp8D;
	ExecuteOp[0x8E] = &Emu6502::ExecuteOp8E;
	ExecuteOp[0x8F] = &Emu6502::ExecuteOp8F;

	ExecuteOp[0x90] = &Emu6502::ExecuteOp90;
	ExecuteOp[0x91] = &Emu6502::ExecuteOp91;
	ExecuteOp[0x92] = &Emu6502::ExecuteOp92;
	ExecuteOp[0x93] = &Emu6502::ExecuteOp93;
	ExecuteOp[0x94] = &Emu6502::ExecuteOp94;
	ExecuteOp[0x95] = &Emu6502::ExecuteOp95;
	ExecuteOp[0x96] = &Emu6502::ExecuteOp96;
	ExecuteOp[0x97] = &Emu6502::ExecuteOp97;
	ExecuteOp[0x98] = &Emu6502::ExecuteOp98;
	ExecuteOp[0x99] = &Emu6502::ExecuteOp99;
	ExecuteOp[0x9A] = &Emu6502::ExecuteOp9A;
	ExecuteOp[0x9B] = &Emu6502::ExecuteOp9B;
	ExecuteOp[0x9C] = &Emu6502::ExecuteOp9C;
	ExecuteOp[0x9D] = &Emu6502::ExecuteOp9D;
	ExecuteOp[0x9E] = &Emu6502::ExecuteOp9E;
	ExecuteOp[0x9F] = &Emu6502::ExecuteOp9F;

	ExecuteOp[0xA0] = &Emu6502::ExecuteOpA0;
	ExecuteOp[0xA1] = &Emu6502::ExecuteOpA1;
	ExecuteOp[0xA2] = &Emu6502::ExecuteOpA2;
	ExecuteOp[0xA3] = &Emu6502::ExecuteOpA3;
	ExecuteOp[0xA4] = &Emu6502::ExecuteOpA4;
	ExecuteOp[0xA5] = &Emu6502::ExecuteOpA5;
	ExecuteOp[0xA6] = &Emu6502::ExecuteOpA6;
	ExecuteOp[0xA7] = &Emu6502::ExecuteOpA7;
	ExecuteOp[0xA8] = &Emu6502::ExecuteOpA8;
	ExecuteOp[0xA9] = &Emu6502::ExecuteOpA9;
	ExecuteOp[0xAA] = &Emu6502::ExecuteOpAA;
	ExecuteOp[0xAB] = &Emu6502::ExecuteOpAB;
	ExecuteOp[0xAC] = &Emu6502::ExecuteOpAC;
	ExecuteOp[0xAD] = &Emu6502::ExecuteOpAD;
	ExecuteOp[0xAE] = &Emu6502::ExecuteOpAE;
	ExecuteOp[0xAF] = &Emu6502::ExecuteOpAF;

	ExecuteOp[0xB0] = &Emu6502::ExecuteOpB0;
	ExecuteOp[0xB1] = &Emu6502::ExecuteOpB1;
	ExecuteOp[0xB2] = &Emu6502::ExecuteOpB2;
	ExecuteOp[0xB3] = &Emu6502::ExecuteOpB3;
	ExecuteOp[0xB4] = &Emu6502::ExecuteOpB4;
	ExecuteOp[0xB5] = &Emu6502::ExecuteOpB5;
	ExecuteOp[0xB6] = &Emu6502::ExecuteOpB6;
	ExecuteOp[0xB7] = &Emu6502::ExecuteOpB7;
	ExecuteOp[0xB8] = &Emu6502::ExecuteOpB8;
	ExecuteOp[0xB9] = &Emu6502::ExecuteOpB9;
	ExecuteOp[0xBA] = &Emu6502::ExecuteOpBA;
	ExecuteOp[0xBB] = &Emu6502::ExecuteOpBB;
	ExecuteOp[0xBC] = &Emu6502::ExecuteOpBC;
	ExecuteOp[0xBD] = &Emu6502::ExecuteOpBD;
	ExecuteOp[0xBE] = &Emu6502::ExecuteOpBE;
	ExecuteOp[0xBF] = &Emu6502::ExecuteOpBF;

	ExecuteOp[0xC0] = &Emu6502::ExecuteOpC0;
	ExecuteOp[0xC1] = &Emu6502::ExecuteOpC1;
	ExecuteOp[0xC2] = &Emu6502::ExecuteOpC2;
	ExecuteOp[0xC3] = &Emu6502::ExecuteOpC3;
	ExecuteOp[0xC4] = &Emu6502::ExecuteOpC4;
	ExecuteOp[0xC5] = &Emu6502::ExecuteOpC5;
	ExecuteOp[0xC6] = &Emu6502::ExecuteOpC6;
	ExecuteOp[0xC7] = &Emu6502::ExecuteOpC7;
	ExecuteOp[0xC8] = &Emu6502::ExecuteOpC8;
	ExecuteOp[0xC9] = &Emu6502::ExecuteOpC9;
	ExecuteOp[0xCA] = &Emu6502::ExecuteOpCA;
	ExecuteOp[0xCB] = &Emu6502::ExecuteOpCB;
	ExecuteOp[0xCC] = &Emu6502::ExecuteOpCC;
	ExecuteOp[0xCD] = &Emu6502::ExecuteOpCD;
	ExecuteOp[0xCE] = &Emu6502::ExecuteOpCE;
	ExecuteOp[0xCF] = &Emu6502::ExecuteOpCF;

	ExecuteOp[0xD0] = &Emu6502::ExecuteOpD0;
	ExecuteOp[0xD1] = &Emu6502::ExecuteOpD1;
	ExecuteOp[0xD2] = &Emu6502::ExecuteOpD2;
	ExecuteOp[0xD3] = &Emu6502::ExecuteOpD3;
	ExecuteOp[0xD4] = &Emu6502::ExecuteOpD4;
	ExecuteOp[0xD5] = &Emu6502::ExecuteOpD5;
	ExecuteOp[0xD6] = &Emu6502::ExecuteOpD6;
	ExecuteOp[0xD7] = &Emu6502::ExecuteOpD7;
	ExecuteOp[0xD8] = &Emu6502::ExecuteOpD8;
	ExecuteOp[0xD9] = &Emu6502::ExecuteOpD9;
	ExecuteOp[0xDA] = &Emu6502::ExecuteOpDA;
	ExecuteOp[0xDB] = &Emu6502::ExecuteOpDB;
	ExecuteOp[0xDC] = &Emu6502::ExecuteOpDC;
	ExecuteOp[0xDD] = &Emu6502::ExecuteOpDD;
	ExecuteOp[0xDE] = &Emu6502::ExecuteOpDE;
	ExecuteOp[0xDF] = &Emu6502::ExecuteOpDF;

	ExecuteOp[0xE0] = &Emu6502::ExecuteOpE0;
	ExecuteOp[0xE1] = &Emu6502::ExecuteOpE1;
	ExecuteOp[0xE2] = &Emu6502::ExecuteOpE2;
	ExecuteOp[0xE3] = &Emu6502::ExecuteOpE3;
	ExecuteOp[0xE4] = &Emu6502::ExecuteOpE4;
	ExecuteOp[0xE5] = &Emu6502::ExecuteOpE5;
	ExecuteOp[0xE6] = &Emu6502::ExecuteOpE6;
	ExecuteOp[0xE7] = &Emu6502::ExecuteOpE7;
	ExecuteOp[0xE8] = &Emu6502::ExecuteOpE8;
	ExecuteOp[0xE9] = &Emu6502::ExecuteOpE9;
	ExecuteOp[0xEA] = &Emu6502::ExecuteOpEA;
	ExecuteOp[0xEB] = &Emu6502::ExecuteOpEB;
	ExecuteOp[0xEC] = &Emu6502::ExecuteOpEC;
	ExecuteOp[0xED] = &Emu6502::ExecuteOpED;
	ExecuteOp[0xEE] = &Emu6502::ExecuteOpEE;
	ExecuteOp[0xEF] = &Emu6502::ExecuteOpEF;

	ExecuteOp[0xF0] = &Emu6502::ExecuteOpF0;
	ExecuteOp[0xF1] = &Emu6502::ExecuteOpF1;
	ExecuteOp[0xF2] = &Emu6502::ExecuteOpF2;
	ExecuteOp[0xF3] = &Emu6502::ExecuteOpF3;
	ExecuteOp[0xF4] = &Emu6502::ExecuteOpF4;
	ExecuteOp[0xF5] = &Emu6502::ExecuteOpF5;
	ExecuteOp[0xF6] = &Emu6502::ExecuteOpF6;
	ExecuteOp[0xF7] = &Emu6502::ExecuteOpF7;
	ExecuteOp[0xF8] = &Emu6502::ExecuteOpF8;
	ExecuteOp[0xF9] = &Emu6502::ExecuteOpF9;
	ExecuteOp[0xFA] = &Emu6502::ExecuteOpFA;
	ExecuteOp[0xFB] = &Emu6502::ExecuteOpFB;
	ExecuteOp[0xFC] = &Emu6502::ExecuteOpFC;
	ExecuteOp[0xFD] = &Emu6502::ExecuteOpFD;
	ExecuteOp[0xFE] = &Emu6502::ExecuteOpFE;
	ExecuteOp[0xFF] = &Emu6502::ExecuteOpFF;
}

Emu6502::~Emu6502()
{
	delete memory;
}

byte Emu6502::GetMemory(ushort addr)
{
	return memory->read(addr);
}

void Emu6502::SetMemory(ushort addr, byte value)
{
	memory->write(addr, value);
}

void Emu6502::ResetRun()
{
	ushort addr = (ushort)((GetMemory(0xFFFC) | (GetMemory(0xFFFD) << 8))); // RESET vector
	Execute(addr);
}

#ifndef WIN32
void strcpy_s(char* dest, size_t size, const char* src)
{
	strncpy(dest, src, size);
}

void strcat_s(char* dest, size_t size, const char* src)
{
	strncat(dest, src, size);
}
#endif

void Emu6502::PHP()
{
	int flags = (N ? 0x80 : 0)
		| (V ? 0x40 : 0)
		| 0x20 // reserved, always set
		| 0x10 // break always set when push
		| (D ? 0x08 : 0)
		| (I ? 0x04 : 0)
		| (Z ? 0x02 : 0)
		| (C ? 0x01 : 0);
	Push(flags);
}

byte Emu6502::LO(ushort value)
{
	return (byte)value;
}

byte Emu6502::HI(ushort value)
{
	return (byte)(value >> 8);
}

byte Emu6502::Subtract(byte reg, byte value, bool *p_overflow)
{
	bool old_reg_neg = (reg & 0x80) != 0;
	bool value_neg = (value & 0x80) != 0;
	int result = reg - value - (C ? 0 : 1);
	N = (result & 0x80) != 0;
	C = (result >= 0);
	Z = (result == 0);
	bool result_neg = (result & 0x80) != 0;
	*p_overflow = (old_reg_neg && !value_neg && !result_neg) // neg - pos = pos
		|| (!old_reg_neg && value_neg && result_neg); // pos - neg = neg
	return (byte)result;
}

byte Emu6502::SubtractWithoutOverflow(byte reg, byte value)
{
	C = true; // init for CMP, etc.
	bool unused;
	return Subtract(reg, value, &unused);
}

void Emu6502::CMP(byte value)
{
	SubtractWithoutOverflow(A, value);
}

void Emu6502::CPX(byte value)
{
	SubtractWithoutOverflow(X, value);
}

void Emu6502::CPY(byte value)
{
	SubtractWithoutOverflow(Y, value);
}

void Emu6502::SetReg(byte *p_reg, int value)
{
	*p_reg = (byte)value;
	Z = (*p_reg == 0);
	N = ((*p_reg & 0x80) != 0);
}

void Emu6502::SetA(int value)
{
	SetReg(&A, value);
}

void Emu6502::SetX(int value)
{
	SetReg(&X, value);
}

void Emu6502::SetY(int value)
{
	SetReg(&Y, value);
}

void Emu6502::SBC(byte value)
{
	if (D)
	{
		int A_dec = (A & 0xF) + ((A >> 4) * 10);
		int value_dec = (value & 0xF) + ((value >> 4) * 10);
		int result_dec = A_dec - value_dec - (C ? 0 : 1);
		C = (result_dec >= 0);
		if (!C)
			result_dec += 100; // wrap negative value
		int result = (result_dec % 10) | (((result_dec / 10) % 10) << 4);
		SetA(result);
		N = false; // undefined?
		V = false; // undefined?
	}
	else
	{
		byte result = Subtract(A, value, &V);
		SetA(result);
	}
}

void Emu6502::ADC(byte value)
{
	int result;
	if (D)
	{
		int A_dec = (A & 0xF) + ((A >> 4) * 10);
		int value_dec = (value & 0xF) + ((value >> 4) * 10);
		int result_dec = A_dec + value_dec + (C ? 1 : 0);
		C = (result_dec > 99);
		result = (result_dec % 10) | (((result_dec / 10) % 10) << 4);
		SetA(result);
		Z = (result_dec == 0); // BCD quirk -- 100 doesn't set Z
		V = false;
	}
	else
	{
		bool A_old_neg = (A & 0x80) != 0;
		bool value_neg = (value & 0x80) != 0;
		result = A + value + (C ? 1 : 0);
		C = (result & 0x100) != 0;
		SetA(result);
		bool result_neg = (result & 0x80) != 0;
		V = (!A_old_neg && !value_neg && result_neg) // pos + pos = neg: overflow
			|| (A_old_neg && value_neg && !result_neg); // neg + neg = pos: overflow
	}
}

void Emu6502::ORA(int value)
{
	SetA(A | value);
}

void Emu6502::EOR(int value)
{
	SetA(A ^ value);
}

void Emu6502::AND(int value)
{
	SetA(A & value);
}

void Emu6502::BIT(byte value)
{
	Z = (A & value) == 0;
	N = (value & 0x80) != 0;
	V = (value & 0x40) != 0;
}

byte Emu6502::ASL(int value)
{
	C = (value & 0x80) != 0;
	value = (byte)(value << 1);
	Z = (value == 0);
	N = (value & 0x80) != 0;
	return (byte)value;
}

byte Emu6502::LSR(int value)
{
	C = (value & 0x01) != 0;
	value = (byte)(value >> 1);
	Z = (value == 0);
	N = false;
	return (byte)value;
}

byte Emu6502::ROL(int value)
{
	bool newC = (value & 0x80) != 0;
	value = (byte)((value << 1) | (C ? 1 : 0));
	C = newC;
	Z = (value == 0);
	N = (value & 0x80) != 0;
	return (byte)value;
}

byte Emu6502::ROR(int value)
{
	bool newC = (value & 0x01) != 0;
	N = C;
	value = (byte)((value >> 1) | (C ? 0x80 : 0));
	C = newC;
	Z = (value == 0);
	return (byte)value;
}

void Emu6502::Push(int value)
{
	SetMemory((ushort)(0x100 + (S--)), (byte)value);
}

byte Emu6502::Pop(void)
{
	return GetMemory((ushort)(0x100 + (++S)));
}

void Emu6502::PLP()
{
	int flags = Pop();
	N = (flags & 0x80) != 0;
	V = (flags & 0x40) != 0;
	B = (flags & 0x10) != 0;
	D = (flags & 0x08) != 0;
	I = (flags & 0x04) != 0;
	Z = (flags & 0x02) != 0;
	C = (flags & 0x01) != 0;
}

void Emu6502::PHA()
{
	Push(A);
}

void Emu6502::PLA()
{
	SetA(Pop());
}

void Emu6502::CLC()
{
	C = false;
}

void Emu6502::CLD()
{
	D = false;
}

void Emu6502::CLI()
{
	I = false;
}

void Emu6502::CLV()
{
	V = false;
}

void Emu6502::SEC()
{
	C = true;
}

void Emu6502::SED()
{
	D = true;
}

void Emu6502::SEI()
{
	I = true;
}

byte Emu6502::INC(byte value)
{
	++value;
	Z = (value == 0);
	N = (value & 0x80) != 0;
	return (byte)value;
}

void Emu6502::INX()
{
	X = INC(X);
}

void Emu6502::INY()
{
	Y = INC(Y);
}

byte Emu6502::DEC(byte value)
{
	--value;
	Z = (value == 0);
	N = (value & 0x80) != 0;
	return (byte)value;
}

void Emu6502::DEX()
{
	X = DEC(X);
}

void Emu6502::DEY()
{
	Y = DEC(Y);
}

void Emu6502::NOP()
{
}

void Emu6502::TXA()
{
	SetReg(&A, X);
}

void Emu6502::TAX()
{
	SetReg(&X, A);
}

void Emu6502::TYA()
{
	SetReg(&A, Y);
}

void Emu6502::TAY()
{
	SetReg(&Y, A);
}

void Emu6502::TXS()
{
	S = X;
}

void Emu6502::TSX()
{
	SetReg(&X, S);
}

ushort Emu6502::GetBR(ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	sbyte offset = (sbyte)GetMemory((ushort)(addr + 1));
	ushort addr2 = (ushort)(addr + 2 + offset);
	return addr2;
}

void Emu6502::BR(bool branch, ushort *p_addr, byte *p_bytes)
{
	ushort addr2 = GetBR(*p_addr, p_bytes);
	if (branch)
	{
		*p_addr = addr2;
		*p_bytes = 0; // don't advance addr
	}
}

void Emu6502::BPL(ushort *p_addr, byte *p_bytes)
{
	BR(!N, p_addr, p_bytes);
}

void Emu6502::BMI(ushort *p_addr, byte *p_bytes)
{
	BR(N, p_addr, p_bytes);
}

void Emu6502::BCC(ushort *p_addr, byte *p_bytes)
{
	BR(!C, p_addr, p_bytes);
}

void Emu6502::BCS(ushort *p_addr, byte *p_bytes)
{
	BR(C, p_addr, p_bytes);
}

void Emu6502::BVC(ushort *p_addr, byte *p_bytes)
{
	BR(!V, p_addr, p_bytes);
}

void Emu6502::BVS(ushort *p_addr, byte *p_bytes)
{
	BR(V, p_addr, p_bytes);
}

void Emu6502::BNE(ushort *p_addr, byte *p_bytes)
{
	BR(!Z, p_addr, p_bytes);
}

void Emu6502::BEQ(ushort *p_addr, byte *p_bytes)
{
	BR(Z, p_addr, p_bytes);
}

void Emu6502::JSR(ushort *p_addr, byte *p_bytes)
{
	*p_bytes = 3; // for next calculation
	ushort addr2 = (ushort)(*p_addr + *p_bytes - 1);
	ushort addr3 = (ushort)(GetMemory((ushort)(*p_addr + 1)) | (GetMemory((ushort)(*p_addr + 2)) << 8));
	Push(HI(addr2));
	Push(LO(addr2));
	*p_addr = addr3;
	*p_bytes = 0; // addr already changed
}

void Emu6502::RTS(ushort *p_addr, byte *p_bytes)
{
	byte lo = Pop();
	byte hi = Pop();
	*p_addr = (ushort)(((hi << 8) | lo) + 1);
	*p_bytes = 0; // addr already changed
}

void Emu6502::RTI(ushort *p_addr, byte *p_bytes)
{
	PLP();
	byte lo = Pop();
	byte hi = Pop();
	*p_bytes = 0; // make sure caller does not increase addr by one
	*p_addr = (ushort)((hi << 8) | lo);
}

void Emu6502::BRK(byte *p_bytes)
{
	++PC;
	++PC;
	Push(HI(PC));
	Push(LO(PC));
	B = true;
	PHP();
	I = true;
	PC = (ushort)(GetMemory(0xFFFE) + (GetMemory(0xFFFF) << 8)); // JMP(IRQ)
	*p_bytes = 0;
}

void Emu6502::JMP(ushort *p_addr, byte *p_bytes)
{
	*p_bytes = 0; // caller should not advance address
	ushort addr2 = (ushort)(GetMemory((ushort)(*p_addr + 1)) | (GetMemory((ushort)(*p_addr + 2)) << 8));
	*p_addr = addr2;
}

void Emu6502::JMPIND(ushort *p_addr, byte *p_bytes)
{
	*p_bytes = 0; // caller should not advance address
	ushort addr2 = (ushort)(GetMemory((ushort)(*p_addr + 1)) | (GetMemory((ushort)(*p_addr + 2)) << 8));
	ushort addr3;
	if ((addr2 & 0xFF) == 0xFF) // JMP($XXFF) won't go over page boundary
		addr3 = (ushort)(GetMemory(addr2) | (GetMemory((ushort)(addr2 - 0xFF)) << 8)); // 6502 "bug" - will use XXFF and XX00 as source of address
	else
		addr3 = (ushort)(GetMemory(addr2) | (GetMemory((ushort)(addr2 + 1)) << 8));
	*p_addr = addr3;
}

// "A:FF X:FF Y:FF S:FF P:XX-XXXXX"
void Emu6502::GetDisplayState(char *state, int state_size)
{
	snprintf(state, state_size, "A:%02X X:%02X Y:%02X S:%02X P:%c%c-%c%c%c%c%c",
		A,
		X,
		Y,
		S,
		N ? 'N' : ' ',
		V ? 'V' : ' ',
		B ? 'B' : ' ',
		D ? 'D' : ' ',
		I ? 'I' : ' ',
		Z ? 'Z' : ' ',
		C ? 'C' : ' '
	);
}

byte Emu6502::GetIndX(ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	byte zpaddr = (byte)(GetMemory((ushort)(addr + 1)) + X); // address must be within zero page
	return GetMemory((ushort)(GetMemory(zpaddr) | (GetMemory((byte)(zpaddr+1)) << 8))); // must keep zpaddr+1 within zero page (byte address)
}

void Emu6502::SetIndX(byte value, ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	byte zpaddr = (byte)(GetMemory((ushort)(addr + 1)) + X); // address must be within zero page
	ushort addr3 = (ushort)(GetMemory(zpaddr) | (GetMemory((byte)(zpaddr+1)) << 8)); // must keep zpaddr+1 within zero page (byte address)

	SetMemory(addr3, value);
}

byte Emu6502::GetIndY(ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = (ushort)(GetMemory((ushort)(addr + 1)));
	ushort addr3 = (ushort)((GetMemory(addr2) | (GetMemory((ushort)(addr2 + 1)) << 8)) + Y);
	return GetMemory(addr3);
}

void Emu6502::SetIndY(byte value, ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = (ushort)(GetMemory((ushort)(addr + 1)));
	ushort addr3 = (ushort)((GetMemory(addr2) | (GetMemory((ushort)(addr2 + 1)) << 8)) + Y);
	SetMemory(addr3, value);
}

byte Emu6502::GetZP(ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = GetMemory((ushort)(addr + 1));
	return GetMemory(addr2);
}

void Emu6502::SetZP(byte value, ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = GetMemory((ushort)(addr + 1));
	SetMemory(addr2, value);
}

byte Emu6502::GetZPX(ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = GetMemory((ushort)(addr + 1));
	return GetMemory((byte)(addr2 + X));
}

void Emu6502::SetZPX(byte value, ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = GetMemory((ushort)(addr + 1));
	SetMemory((byte)(addr2 + X), value);
}

byte Emu6502::GetZPY(ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = GetMemory((ushort)(addr + 1));
	return GetMemory((byte)(addr2 + Y));
}

void Emu6502::SetZPY(byte value, ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = GetMemory((ushort)(addr + 1));
	SetMemory((byte)(addr2 + Y), value);
}

byte Emu6502::GetABS(ushort addr, byte *p_bytes)
{
	*p_bytes = 3;
	ushort addr2 = (ushort)(GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
	return GetMemory(addr2);
}

void Emu6502::SetABS(byte value, ushort addr, byte *p_bytes)
{
	*p_bytes = 3;
	ushort addr2 = (ushort)(GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
	SetMemory(addr2, value);
}

byte Emu6502::GetABSX(ushort addr, byte *p_bytes)
{
	*p_bytes = 3;
	ushort addr2 = (ushort)(GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
	return GetMemory((ushort)(addr2 + X));
}

void Emu6502::SetABSX(byte value, ushort addr, byte *p_bytes)
{
	*p_bytes = 3;
	ushort addr2 = (ushort)((GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8)) + X);
	SetMemory(addr2, value);
}

byte Emu6502::GetABSY(ushort addr, byte *p_bytes)
{
	*p_bytes = 3;
	ushort addr2 = (ushort)(GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
	return GetMemory((ushort)(addr2 + Y));
}

void Emu6502::SetABSY(byte value, ushort addr, byte *p_bytes)
{
	*p_bytes = 3;
	ushort addr2 = (ushort)((GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8)) + Y);
	SetMemory(addr2, value);
}

byte Emu6502::GetIM(ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	return GetMemory((ushort)(addr + 1));
}

void Emu6502::Execute(ushort addr)
{
	bool conditional;
	byte bytes;

	PC = addr;
	bool breakpoint = false;

	while (true)
	{
		while (true)
		{
			if (quit)
				return;
			bytes = 1;
			//if (Breakpoints.Contains(PC))
			//	breakpoint = true;
			if (trace || breakpoint || step)
			{
				ushort addr2;
				char line[27];
				char dis[13];
				DisassembleLong(PC, &conditional, &bytes, &addr2, dis, sizeof(dis), line, sizeof(line));
				char state[33];
				GetDisplayState(state, sizeof(state));
				char full_line[80];
				snprintf(full_line, sizeof(full_line), "%-30s%s\n", line, state);
#ifdef WIN32
				OutputDebugStringA(full_line);
#else				
				fprintf(stderr, "%s", full_line);
#endif				
				if (step)
					step = step; // user can put debug breakpoint here to allow stepping
				if (breakpoint)
					breakpoint = breakpoint; // user can put debug breakpoint here to allow break;
			}
			if (!ExecutePatch()) // allow execute to be overriden at a specific address
				break;
		}

		void (Emu6502::*pMethod)(byte * p_bytes) = ExecuteOp[GetMemory(PC)];
		((*this).*pMethod)(&bytes);
		PC += bytes;
	}
}

void Emu6502::ExecuteOp00(byte* p_bytes) { BRK(p_bytes); }
void Emu6502::ExecuteOp01(byte* p_bytes) { ORA(GetIndX(PC, p_bytes)); }
void Emu6502::ExecuteOp02(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp03(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp04(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp05(byte* p_bytes) { ORA(GetZP(PC, p_bytes)); }
void Emu6502::ExecuteOp06(byte* p_bytes) { SetZP(ASL(GetZP(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOp07(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp08(byte* p_bytes) { PHP(); }
void Emu6502::ExecuteOp09(byte* p_bytes) { ORA(GetIM(PC, p_bytes)); }
void Emu6502::ExecuteOp0A(byte* p_bytes) { SetA(ASL(A)); }
void Emu6502::ExecuteOp0B(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp0C(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp0D(byte* p_bytes) { ORA(GetABS(PC, p_bytes)); }
void Emu6502::ExecuteOp0E(byte* p_bytes) { SetABS(ASL(GetABS(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOp0F(byte* p_bytes) { InvalidOpcode(); }

void Emu6502::ExecuteOp10(byte* p_bytes) { BPL(&PC, p_bytes); }
void Emu6502::ExecuteOp11(byte* p_bytes) { ORA(GetIndY(PC, p_bytes)); }
void Emu6502::ExecuteOp12(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp13(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp14(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp15(byte* p_bytes) { ORA(GetZPX(PC, p_bytes)); }
void Emu6502::ExecuteOp16(byte* p_bytes) { SetZPX(ASL(GetZPX(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOp17(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp18(byte* p_bytes) { CLC(); }
void Emu6502::ExecuteOp19(byte* p_bytes) { ORA(GetABSY(PC, p_bytes)); }
void Emu6502::ExecuteOp1A(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp1B(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp1C(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp1D(byte* p_bytes) { ORA(GetABSX(PC, p_bytes)); }
void Emu6502::ExecuteOp1E(byte* p_bytes) { SetABSX(ASL(GetABSX(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOp1F(byte* p_bytes) { InvalidOpcode(); }

void Emu6502::ExecuteOp20(byte* p_bytes) { JSR(&PC, p_bytes); }
void Emu6502::ExecuteOp21(byte* p_bytes) { AND(GetIndX(PC, p_bytes)); }
void Emu6502::ExecuteOp22(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp23(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp24(byte* p_bytes) { BIT(GetZP(PC, p_bytes)); }
void Emu6502::ExecuteOp25(byte* p_bytes) { AND(GetZP(PC, p_bytes)); }
void Emu6502::ExecuteOp26(byte* p_bytes) { SetZP(ROL(GetZP(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOp27(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp28(byte* p_bytes) { PLP(); }
void Emu6502::ExecuteOp29(byte* p_bytes) { AND(GetIM(PC, p_bytes)); }
void Emu6502::ExecuteOp2A(byte* p_bytes) { SetA(ROL(A)); }
void Emu6502::ExecuteOp2B(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp2C(byte* p_bytes) { BIT(GetABS(PC, p_bytes)); }
void Emu6502::ExecuteOp2D(byte* p_bytes) { AND(GetABS(PC, p_bytes)); }
void Emu6502::ExecuteOp2E(byte* p_bytes) { SetABS(ROL(GetABS(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOp2F(byte* p_bytes) { InvalidOpcode(); }

void Emu6502::ExecuteOp30(byte* p_bytes) { BMI(&PC, p_bytes); }
void Emu6502::ExecuteOp31(byte* p_bytes) { AND(GetIndY(PC, p_bytes)); }
void Emu6502::ExecuteOp32(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp33(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp34(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp35(byte* p_bytes) { AND(GetZPX(PC, p_bytes)); }
void Emu6502::ExecuteOp36(byte* p_bytes) { SetZPX(ROL(GetZPX(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOp37(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp38(byte* p_bytes) { SEC(); }
void Emu6502::ExecuteOp39(byte* p_bytes) { AND(GetABSY(PC, p_bytes)); }
void Emu6502::ExecuteOp3A(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp3B(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp3C(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp3D(byte* p_bytes) { AND(GetABSX(PC, p_bytes)); }
void Emu6502::ExecuteOp3E(byte* p_bytes) { SetABSX(ROL(GetABSX(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOp3F(byte* p_bytes) { InvalidOpcode(); }

void Emu6502::ExecuteOp40(byte* p_bytes) { RTI(&PC, p_bytes); }
void Emu6502::ExecuteOp41(byte* p_bytes) { EOR(GetIndX(PC, p_bytes)); }
void Emu6502::ExecuteOp42(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp43(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp44(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp45(byte* p_bytes) { EOR(GetZP(PC, p_bytes)); }
void Emu6502::ExecuteOp46(byte* p_bytes) { SetZP(LSR(GetZP(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOp47(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp48(byte* p_bytes) { PHA(); }
void Emu6502::ExecuteOp49(byte* p_bytes) { EOR(GetIM(PC, p_bytes)); }
void Emu6502::ExecuteOp4A(byte* p_bytes) { SetA(LSR(A)); }
void Emu6502::ExecuteOp4B(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp4C(byte* p_bytes) { JMP(&PC, p_bytes); }
void Emu6502::ExecuteOp4D(byte* p_bytes) { EOR(GetABS(PC, p_bytes)); }
void Emu6502::ExecuteOp4E(byte* p_bytes) { SetABS(LSR(GetABS(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOp4F(byte* p_bytes) { InvalidOpcode(); }

void Emu6502::ExecuteOp50(byte* p_bytes) { BVC(&PC, p_bytes); }
void Emu6502::ExecuteOp51(byte* p_bytes) { EOR(GetIndY(PC, p_bytes)); }
void Emu6502::ExecuteOp52(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp53(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp54(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp55(byte* p_bytes) { EOR(GetZPX(PC, p_bytes)); }
void Emu6502::ExecuteOp56(byte* p_bytes) { SetZPX(LSR(GetZPX(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOp57(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp58(byte* p_bytes) { CLI(); }
void Emu6502::ExecuteOp59(byte* p_bytes) { EOR(GetABSY(PC, p_bytes)); }
void Emu6502::ExecuteOp5A(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp5B(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp5C(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp5D(byte* p_bytes) { EOR(GetABSX(PC, p_bytes)); }
void Emu6502::ExecuteOp5E(byte* p_bytes) { SetABSX(LSR(GetABSX(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOp5F(byte* p_bytes) { InvalidOpcode(); }

void Emu6502::ExecuteOp60(byte* p_bytes) { RTS(&PC, p_bytes); }
void Emu6502::ExecuteOp61(byte* p_bytes) { ADC(GetIndX(PC, p_bytes)); }
void Emu6502::ExecuteOp62(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp63(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp64(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp65(byte* p_bytes) { ADC(GetZP(PC, p_bytes)); }
void Emu6502::ExecuteOp66(byte* p_bytes) { SetZP(ROR(GetZP(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOp67(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp68(byte* p_bytes) { PLA(); }
void Emu6502::ExecuteOp69(byte* p_bytes) { ADC(GetIM(PC, p_bytes)); }
void Emu6502::ExecuteOp6A(byte* p_bytes) { SetA(ROR(A)); }
void Emu6502::ExecuteOp6B(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp6C(byte* p_bytes) { JMPIND(&PC, p_bytes); }
void Emu6502::ExecuteOp6D(byte* p_bytes) { ADC(GetABS(PC, p_bytes)); }
void Emu6502::ExecuteOp6E(byte* p_bytes) { SetABS(ROR(GetABS(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOp6F(byte* p_bytes) { InvalidOpcode(); }

void Emu6502::ExecuteOp70(byte* p_bytes) { BVS(&PC, p_bytes); }
void Emu6502::ExecuteOp71(byte* p_bytes) { ADC(GetIndY(PC, p_bytes)); }
void Emu6502::ExecuteOp72(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp73(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp74(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp75(byte* p_bytes) { ADC(GetZPX(PC, p_bytes)); }
void Emu6502::ExecuteOp76(byte* p_bytes) { SetZPX(ROR(GetZPX(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOp77(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp78(byte* p_bytes) { SEI(); }
void Emu6502::ExecuteOp79(byte* p_bytes) { ADC(GetABSY(PC, p_bytes)); }
void Emu6502::ExecuteOp7A(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp7B(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp7C(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp7D(byte* p_bytes) { ADC(GetABSX(PC, p_bytes)); }
void Emu6502::ExecuteOp7E(byte* p_bytes) { SetABSX(ROR(GetABSX(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOp7F(byte* p_bytes) { InvalidOpcode(); }

void Emu6502::ExecuteOp80(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp81(byte* p_bytes) { SetIndX(A, PC, p_bytes); }
void Emu6502::ExecuteOp82(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp83(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp84(byte* p_bytes) { SetZP(Y, PC, p_bytes); }
void Emu6502::ExecuteOp85(byte* p_bytes) { SetZP(A, PC, p_bytes); }
void Emu6502::ExecuteOp86(byte* p_bytes) { SetZP(X, PC, p_bytes); }
void Emu6502::ExecuteOp87(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp88(byte* p_bytes) { DEY(); }
void Emu6502::ExecuteOp89(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp8A(byte* p_bytes) { TXA(); }
void Emu6502::ExecuteOp8B(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp8C(byte* p_bytes) { SetABS(Y, PC, p_bytes); }
void Emu6502::ExecuteOp8D(byte* p_bytes) { SetABS(A, PC, p_bytes); }
void Emu6502::ExecuteOp8E(byte* p_bytes) { SetABS(X, PC, p_bytes); }
void Emu6502::ExecuteOp8F(byte* p_bytes) { InvalidOpcode(); }

void Emu6502::ExecuteOp90(byte* p_bytes) { BCC(&PC, p_bytes); }
void Emu6502::ExecuteOp91(byte* p_bytes) { SetIndY(A, PC, p_bytes); }
void Emu6502::ExecuteOp92(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp93(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp94(byte* p_bytes) { SetZPX(Y, PC, p_bytes); }
void Emu6502::ExecuteOp95(byte* p_bytes) { SetZPX(A, PC, p_bytes); }
void Emu6502::ExecuteOp96(byte* p_bytes) { SetZPY(X, PC, p_bytes); }
void Emu6502::ExecuteOp97(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp98(byte* p_bytes) { TYA(); }
void Emu6502::ExecuteOp99(byte* p_bytes) { SetABSY(A, PC, p_bytes); }
void Emu6502::ExecuteOp9A(byte* p_bytes) { TXS(); }
void Emu6502::ExecuteOp9B(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp9C(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp9D(byte* p_bytes) { SetABSX(A, PC, p_bytes); }
void Emu6502::ExecuteOp9E(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOp9F(byte* p_bytes) { InvalidOpcode(); }

void Emu6502::ExecuteOpA0(byte* p_bytes) { SetY(GetIM(PC, p_bytes)); }
void Emu6502::ExecuteOpA1(byte* p_bytes) { SetA(GetIndX(PC, p_bytes)); }
void Emu6502::ExecuteOpA2(byte* p_bytes) { SetX(GetIM(PC, p_bytes)); }
void Emu6502::ExecuteOpA3(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpA4(byte* p_bytes) { SetY(GetZP(PC, p_bytes)); }
void Emu6502::ExecuteOpA5(byte* p_bytes) { SetA(GetZP(PC, p_bytes)); }
void Emu6502::ExecuteOpA6(byte* p_bytes) { SetX(GetZP(PC, p_bytes)); }
void Emu6502::ExecuteOpA7(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpA8(byte* p_bytes) { TAY(); }
void Emu6502::ExecuteOpA9(byte* p_bytes) { SetA(GetIM(PC, p_bytes)); }
void Emu6502::ExecuteOpAA(byte* p_bytes) { TAX(); }
void Emu6502::ExecuteOpAB(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpAC(byte* p_bytes) { SetY(GetABS(PC, p_bytes)); }
void Emu6502::ExecuteOpAD(byte* p_bytes) { SetA(GetABS(PC, p_bytes)); }
void Emu6502::ExecuteOpAE(byte* p_bytes) { SetX(GetABS(PC, p_bytes)); }
void Emu6502::ExecuteOpAF(byte* p_bytes) { InvalidOpcode(); }

void Emu6502::ExecuteOpB0(byte* p_bytes) { BCS(&PC, p_bytes); }
void Emu6502::ExecuteOpB1(byte* p_bytes) { SetA(GetIndY(PC, p_bytes)); }
void Emu6502::ExecuteOpB2(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpB3(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpB4(byte* p_bytes) { SetY(GetZPX(PC, p_bytes)); }
void Emu6502::ExecuteOpB5(byte* p_bytes) { SetA(GetZPX(PC, p_bytes)); }
void Emu6502::ExecuteOpB6(byte* p_bytes) { SetX(GetZPY(PC, p_bytes)); }
void Emu6502::ExecuteOpB7(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpB8(byte* p_bytes) { CLV(); }
void Emu6502::ExecuteOpB9(byte* p_bytes) { SetA(GetABSY(PC, p_bytes)); }
void Emu6502::ExecuteOpBA(byte* p_bytes) { TSX(); }
void Emu6502::ExecuteOpBB(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpBC(byte* p_bytes) { SetY(GetABSX(PC, p_bytes)); }
void Emu6502::ExecuteOpBD(byte* p_bytes) { SetA(GetABSX(PC, p_bytes)); }
void Emu6502::ExecuteOpBE(byte* p_bytes) { SetX(GetABSY(PC, p_bytes)); }
void Emu6502::ExecuteOpBF(byte* p_bytes) { InvalidOpcode(); }

void Emu6502::ExecuteOpC0(byte* p_bytes) { CPY(GetIM(PC, p_bytes)); }
void Emu6502::ExecuteOpC1(byte* p_bytes) { CMP(GetIndX(PC, p_bytes)); }
void Emu6502::ExecuteOpC2(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpC3(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpC4(byte* p_bytes) { CPY(GetZP(PC, p_bytes)); }
void Emu6502::ExecuteOpC5(byte* p_bytes) { CMP(GetZP(PC, p_bytes)); }
void Emu6502::ExecuteOpC6(byte* p_bytes) { SetZP(DEC(GetZP(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOpC7(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpC8(byte* p_bytes) { INY(); }
void Emu6502::ExecuteOpC9(byte* p_bytes) { CMP(GetIM(PC, p_bytes)); }
void Emu6502::ExecuteOpCA(byte* p_bytes) { DEX(); }
void Emu6502::ExecuteOpCB(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpCC(byte* p_bytes) { CPY(GetABS(PC, p_bytes)); }
void Emu6502::ExecuteOpCD(byte* p_bytes) { CMP(GetABS(PC, p_bytes)); }
void Emu6502::ExecuteOpCE(byte* p_bytes) { SetABS(DEC(GetABS(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOpCF(byte* p_bytes) { InvalidOpcode(); }

void Emu6502::ExecuteOpD0(byte* p_bytes) { BNE(&PC, p_bytes); }
void Emu6502::ExecuteOpD1(byte* p_bytes) { CMP(GetIndY(PC, p_bytes)); }
void Emu6502::ExecuteOpD2(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpD3(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpD4(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpD5(byte* p_bytes) { CMP(GetZPX(PC, p_bytes)); }
void Emu6502::ExecuteOpD6(byte* p_bytes) { SetZPX(DEC(GetZPX(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOpD7(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpD8(byte* p_bytes) { CLD(); }
void Emu6502::ExecuteOpD9(byte* p_bytes) { CMP(GetABSY(PC, p_bytes)); }
void Emu6502::ExecuteOpDA(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpDB(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpDC(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpDD(byte* p_bytes) { CMP(GetABSX(PC, p_bytes)); }
void Emu6502::ExecuteOpDE(byte* p_bytes) { SetABSX(DEC(GetABSX(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOpDF(byte* p_bytes) { InvalidOpcode(); }

void Emu6502::ExecuteOpE0(byte* p_bytes) { CPX(GetIM(PC, p_bytes)); }
void Emu6502::ExecuteOpE1(byte* p_bytes) { SBC(GetIndX(PC, p_bytes)); }
void Emu6502::ExecuteOpE2(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpE3(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpE4(byte* p_bytes) { CPX(GetZP(PC, p_bytes)); }
void Emu6502::ExecuteOpE5(byte* p_bytes) { SBC(GetZP(PC, p_bytes)); }
void Emu6502::ExecuteOpE6(byte* p_bytes) { SetZP(INC(GetZP(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOpE7(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpE8(byte* p_bytes) { INX(); }
void Emu6502::ExecuteOpE9(byte* p_bytes) { SBC(GetIM(PC, p_bytes)); }
void Emu6502::ExecuteOpEA(byte* p_bytes) { NOP(); }
void Emu6502::ExecuteOpEB(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpEC(byte* p_bytes) { CPX(GetABS(PC, p_bytes)); }
void Emu6502::ExecuteOpED(byte* p_bytes) { SBC(GetABS(PC, p_bytes)); }
void Emu6502::ExecuteOpEE(byte* p_bytes) { SetABS(INC(GetABS(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOpEF(byte* p_bytes) { InvalidOpcode(); }

void Emu6502::ExecuteOpF0(byte* p_bytes) { BEQ(&PC, p_bytes); }
void Emu6502::ExecuteOpF1(byte* p_bytes) { SBC(GetIndY(PC, p_bytes)); }
void Emu6502::ExecuteOpF2(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpF3(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpF4(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpF5(byte* p_bytes) { SBC(GetZPX(PC, p_bytes)); }
void Emu6502::ExecuteOpF6(byte* p_bytes) { SetZPX(INC(GetZPX(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOpF7(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpF8(byte* p_bytes) { SED(); }
void Emu6502::ExecuteOpF9(byte* p_bytes) { SBC(GetABSY(PC, p_bytes)); }
void Emu6502::ExecuteOpFA(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpFB(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpFC(byte* p_bytes) { InvalidOpcode(); }
void Emu6502::ExecuteOpFD(byte* p_bytes) { SBC(GetABSX(PC, p_bytes)); }
void Emu6502::ExecuteOpFE(byte* p_bytes) { SetABSX(INC(GetABSX(PC, p_bytes)), PC, p_bytes); }
void Emu6502::ExecuteOpFF(byte* p_bytes) { InvalidOpcode(); }

void Emu6502::InvalidOpcode()
{
	printf("Invalid opcode %02X at %04X", GetMemory(PC), PC);
	exit(1);
}

// Examples:
// FFFF FF FF FF JMP ($FFFF)
// FFFF FF FF FF LDA $FFFF,X
void Emu6502::DisassembleLong(ushort addr, bool *p_conditional, byte *p_bytes, ushort *p_addr2, char *dis, int dis_size, char *line, int line_size)
{
	DisassembleShort(addr, p_conditional, p_bytes, p_addr2, dis, dis_size);
	snprintf(line, line_size, "%04X ", addr);
	for (int i = 0; i < 3; ++i)
	{
		if (i < *p_bytes)
			snprintf(line+strlen(line), line_size-strlen(line), "%02X ", GetMemory((ushort)(addr + i)));
		else
			strcat_s(line, line_size, "   ");
	}
	strcat_s(line, line_size, dis);
}

void Emu6502::Ind(char *dis, int dis_size, const char* opcode, ushort addr, ushort *p_addr2, byte *p_bytes)
{
	*p_bytes = 3;
	ushort addr1 = (ushort)(GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
	*p_addr2 = (ushort)(GetMemory(addr1) | (GetMemory((ushort)(addr1 + 1)) << 8));
	snprintf(dis, dis_size, "%s ($%0X4)", opcode, addr1);
}

void Emu6502::IndX(char *dis, int dis_size, const char* opcode, ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	snprintf(dis, dis_size, "%s ($%0X2,X)", opcode, GetMemory((ushort)(addr + 1)));
}

void Emu6502::IndY(char *dis, int dis_size, const char* opcode, ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	snprintf(dis, dis_size, "%s ($%0X2),Y", opcode, GetMemory((ushort)(addr + 1)));
}

void Emu6502::ZP(char *dis, int dis_size, const char* opcode, ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	snprintf(dis, dis_size, "%s $%0X2", opcode, GetMemory((ushort)(addr + 1)));
}

void Emu6502::ZPX(char *dis, int dis_size, const char* opcode, ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	snprintf(dis, dis_size, "%s $%0X2,X", opcode, GetMemory((ushort)(addr + 1)));
}

void Emu6502::ZPY(char *dis, int dis_size, const char* opcode, ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	snprintf(dis, dis_size, "%s $%0X2,Y", opcode, GetMemory((ushort)(addr + 1)));
}

void Emu6502::ABS(char *dis, int dis_size, const char* opcode, ushort addr, byte *p_bytes)
{
	*p_bytes = 3;
	snprintf(dis, dis_size, "%s $%04X", opcode, GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
}

void Emu6502::ABSAddr(char *dis, int dis_size, const char* opcode, ushort addr, ushort *p_addr2, byte *p_bytes)
{
	*p_bytes = 3;
	*p_addr2 = (ushort)(GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
	snprintf(dis, dis_size, "%s $%04X", opcode, *p_addr2);
}

void Emu6502::ABSX(char *dis, int dis_size, const char* opcode, ushort addr, byte *p_bytes)
{
	*p_bytes = 3;
	snprintf(dis, dis_size, "%s $%04X,X", opcode, GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
}

void Emu6502::ABSY(char *dis, int dis_size, const char* opcode, ushort addr, byte *p_bytes)
{
	*p_bytes = 3;
	snprintf(dis, dis_size, "%s $%04X,Y", opcode, GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
}

void Emu6502::IM(char *dis, int dis_size, const char* opcode, ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	snprintf(dis, dis_size, "%s #$%02X", opcode, GetMemory((ushort)(addr + 1)));
}

void Emu6502::BRX(char *dis, int dis_size, const char* opcode, ushort addr, bool *p_conditional, ushort *p_addr2, byte *p_bytes)
{
	*p_bytes = 2;
	*p_conditional = true;
	sbyte offset = (sbyte)GetMemory((ushort)(addr + 1));
	*p_addr2 = (ushort)(addr + 2 + offset);
	snprintf(dis, dis_size, "%s $%04X", opcode, *p_addr2);
}

// JMP ($FFFF)
// LDA $FFFF,X
void Emu6502::DisassembleShort(ushort addr, bool *p_conditional, byte *p_bytes, ushort *p_addr2, char *dis, int dis_size)
{
	*p_conditional = false;
	*p_addr2 = 0;
	*p_bytes = 1;

	switch (GetMemory(addr))
	{
	case 0x00: strcpy_s(dis, dis_size, "BRK"); return;
	case 0x01: IndX(dis, dis_size, "ORA", addr, p_bytes); return;
	case 0x05: ZP(dis, dis_size, "ORA", addr, p_bytes); return;
	case 0x06: ZP(dis, dis_size, "ASL", addr, p_bytes); return;
	case 0x08: strcpy_s(dis, dis_size, "PHP"); return;
	case 0x09: IM(dis, dis_size, "ORA", addr, p_bytes); return;
	case 0x0A: strcpy_s(dis, dis_size, "ASL A"); return;
	case 0x0D: ABS(dis, dis_size, "ORA", addr, p_bytes); return;
	case 0x0E: ABS(dis, dis_size, "ASL", addr, p_bytes); return;

	case 0x10: BRX(dis, dis_size, "BPL", addr, p_conditional, p_addr2, p_bytes); return;
	case 0x11: IndY(dis, dis_size, "ORA", addr, p_bytes); return;
	case 0x15: ZPX(dis, dis_size, "ORA", addr, p_bytes); return;
	case 0x16: ZPX(dis, dis_size, "ASL", addr, p_bytes); return;
	case 0x18: strcpy_s(dis, dis_size, "CLC"); return;
	case 0x19: ABSY(dis, dis_size, "ORA", addr, p_bytes); return;
	case 0x1D: ABSX(dis, dis_size, "ORA", addr, p_bytes); return;
	case 0x1E: ABSX(dis, dis_size, "ASL", addr, p_bytes); return;

	case 0x20: ABSAddr(dis, dis_size, "JSR", addr, p_addr2, p_bytes); return;
	case 0x21: IndX(dis, dis_size, "AND", addr, p_bytes); return;
	case 0x24: ZP(dis, dis_size, "BIT", addr, p_bytes); return;
	case 0x25: ZP(dis, dis_size, "AND", addr, p_bytes); return;
	case 0x26: ZP(dis, dis_size, "ROL", addr, p_bytes); return;
	case 0x28: strcpy_s(dis, dis_size, "PLP"); return;
	case 0x29: IM(dis, dis_size, "AND", addr, p_bytes); return;
	case 0x2A: strcpy_s(dis, dis_size, "ROL A"); return;
	case 0x2C: ABS(dis, dis_size, "BIT", addr, p_bytes); return;
	case 0x2D: ABS(dis, dis_size, "AND", addr, p_bytes); return;
	case 0x2E: ABS(dis, dis_size, "ROL", addr, p_bytes); return;

	case 0x30: BRX(dis, dis_size, "BMI", addr, p_conditional, p_addr2, p_bytes); return;
	case 0x31: IndY(dis, dis_size, "AND", addr, p_bytes); return;
	case 0x35: ZPX(dis, dis_size, "AND", addr, p_bytes); return;
	case 0x36: ZPX(dis, dis_size, "ROL", addr, p_bytes); return;
	case 0x38: strcpy_s(dis, dis_size, "SEC"); return;
	case 0x39: ABSY(dis, dis_size, "AND", addr, p_bytes); return;
	case 0x3D: ABSX(dis, dis_size, "AND", addr, p_bytes); return;
	case 0x3E: ABSX(dis, dis_size, "ROL", addr, p_bytes); return;

	case 0x40: strcpy_s(dis, dis_size, "RTI"); return;
	case 0x41: IndX(dis, dis_size, "EOR", addr, p_bytes); return;
	case 0x45: ZP(dis, dis_size, "EOR", addr, p_bytes); return;
	case 0x46: ZP(dis, dis_size, "LSR", addr, p_bytes); return;
	case 0x48: strcpy_s(dis, dis_size, "PHA"); return;
	case 0x49: IM(dis, dis_size, "EOR", addr, p_bytes); return;
	case 0x4A: strcpy_s(dis, dis_size, "LSR A"); return;
	case 0x4C: ABSAddr(dis, dis_size, "JMP", addr, p_addr2, p_bytes); return;
	case 0x4D: ABS(dis, dis_size, "EOR", addr, p_bytes); return;
	case 0x4E: ABS(dis, dis_size, "LSR", addr, p_bytes); return;

	case 0x50: BRX(dis, dis_size, "BVC", addr, p_conditional, p_addr2, p_bytes); return;
	case 0x51: IndY(dis, dis_size, "EOR", addr, p_bytes); return;
	case 0x55: ZPX(dis, dis_size, "EOR", addr, p_bytes); return;
	case 0x56: ZPX(dis, dis_size, "LSR", addr, p_bytes); return;
	case 0x58: strcpy_s(dis, dis_size, "CLI"); return;
	case 0x59: ABSY(dis, dis_size, "EOR", addr, p_bytes); return;
	case 0x5D: ABSX(dis, dis_size, "EOR", addr, p_bytes); return;
	case 0x5E: ABSX(dis, dis_size, "LSR", addr, p_bytes); return;

	case 0x60: strcpy_s(dis, dis_size, "RTS"); return;
	case 0x61: IndX(dis, dis_size, "ADC", addr, p_bytes); return;
	case 0x65: ZP(dis, dis_size, "ADC", addr, p_bytes); return;
	case 0x66: ZP(dis, dis_size, "ROR", addr, p_bytes); return;
	case 0x68: strcpy_s(dis, dis_size, "PLA"); return;
	case 0x69: IM(dis, dis_size, "ADC", addr, p_bytes); return;
	case 0x6A: strcpy_s(dis, dis_size, "ROR A"); return;
	case 0x6C: Ind(dis, dis_size, "JMP", addr, p_addr2, p_bytes); return;
	case 0x6D: ABS(dis, dis_size, "ADC", addr, p_bytes); return;
	case 0x6E: ABS(dis, dis_size, "ROR", addr, p_bytes); return;

	case 0x70: BRX(dis, dis_size, "BVS", addr, p_conditional, p_addr2, p_bytes); return;
	case 0x71: IndY(dis, dis_size, "ADC", addr, p_bytes); return;
	case 0x75: ZPX(dis, dis_size, "ADC", addr, p_bytes); return;
	case 0x76: ZPX(dis, dis_size, "ROR", addr, p_bytes); return;
	case 0x78: strcpy_s(dis, dis_size, "SEI"); return;
	case 0x79: ABSY(dis, dis_size, "ADC", addr, p_bytes); return;
	case 0x7D: ABSX(dis, dis_size, "ADC", addr, p_bytes); return;
	case 0x7E: ABSX(dis, dis_size, "ROR", addr, p_bytes); return;

	case 0x81: IndX(dis, dis_size, "STA", addr, p_bytes); return;
	case 0x84: ZP(dis, dis_size, "STY", addr, p_bytes); return;
	case 0x85: ZP(dis, dis_size, "STA", addr, p_bytes); return;
	case 0x86: ZP(dis, dis_size, "STX", addr, p_bytes); return;
	case 0x88: strcpy_s(dis, dis_size, "DEY"); return;
	case 0x8A: strcpy_s(dis, dis_size, "TXA"); return;
	case 0x8C: ABS(dis, dis_size, "STY", addr, p_bytes); return;
	case 0x8D: ABS(dis, dis_size, "STA", addr, p_bytes); return;
	case 0x8E: ABS(dis, dis_size, "STX", addr, p_bytes); return;

	case 0x90: BRX(dis, dis_size, "BCC", addr, p_conditional, p_addr2, p_bytes); return;
	case 0x91: IndY(dis, dis_size, "STA", addr, p_bytes); return;
	case 0x94: ZPX(dis, dis_size, "STY", addr, p_bytes); return;
	case 0x95: ZPX(dis, dis_size, "STA", addr, p_bytes); return;
	case 0x96: ZPY(dis, dis_size, "STX", addr, p_bytes); return;
	case 0x98: strcpy_s(dis, dis_size, "TYA"); return;
	case 0x99: ABSY(dis, dis_size, "STA", addr, p_bytes); return;
	case 0x9A: strcpy_s(dis, dis_size, "TXS"); return;
	case 0x9D: ABSX(dis, dis_size, "STA", addr, p_bytes); return;

	case 0xA0: IM(dis, dis_size, "LDY", addr, p_bytes); return;
	case 0xA1: IndX(dis, dis_size, "LDA", addr, p_bytes); return;
	case 0xA2: IM(dis, dis_size, "LDX", addr, p_bytes); return;
	case 0xA4: ZP(dis, dis_size, "LDY", addr, p_bytes); return;
	case 0xA5: ZP(dis, dis_size, "LDA", addr, p_bytes); return;
	case 0xA6: ZP(dis, dis_size, "LDX", addr, p_bytes); return;
	case 0xA8: strcpy_s(dis, dis_size, "TAY"); return;
	case 0xA9: IM(dis, dis_size, "LDA", addr, p_bytes); return;
	case 0xAA: strcpy_s(dis, dis_size, "TAX"); return;
	case 0xAC: ABS(dis, dis_size, "LDY", addr, p_bytes); return;
	case 0xAD: ABS(dis, dis_size, "LDA", addr, p_bytes); return;
	case 0xAE: ABS(dis, dis_size, "LDX", addr, p_bytes); return;

	case 0xB0: BRX(dis, dis_size, "BCS", addr, p_conditional, p_addr2, p_bytes); return;
	case 0xB1: IndY(dis, dis_size, "LDA", addr, p_bytes); return;
	case 0xB4: ZPX(dis, dis_size, "LDY", addr, p_bytes); return;
	case 0xB5: ZPX(dis, dis_size, "LDA", addr, p_bytes); return;
	case 0xB6: ZPY(dis, dis_size, "LDX", addr, p_bytes); return;
	case 0xB8: strcpy_s(dis, dis_size, "CLV"); return;
	case 0xB9: ABSY(dis, dis_size, "LDA", addr, p_bytes); return;
	case 0xBA: strcpy_s(dis, dis_size, "TSX"); return;
	case 0xBC: ABSX(dis, dis_size, "LDY", addr, p_bytes); return;
	case 0xBD: ABSX(dis, dis_size, "LDA", addr, p_bytes); return;
	case 0xBE: ABSY(dis, dis_size, "LDX", addr, p_bytes); return;

	case 0xC0: IM(dis, dis_size, "CPY", addr, p_bytes); return;
	case 0xC1: IndX(dis, dis_size, "CMP", addr, p_bytes); return;
	case 0xC4: ZP(dis, dis_size, "CPY", addr, p_bytes); return;
	case 0xC5: ZP(dis, dis_size, "CMP", addr, p_bytes); return;
	case 0xC6: ZP(dis, dis_size, "DEC", addr, p_bytes); return;
	case 0xC8: strcpy_s(dis, dis_size, "INY"); return;
	case 0xC9: IM(dis, dis_size, "CMP", addr, p_bytes); return;
	case 0xCA: strcpy_s(dis, dis_size, "DEX"); return;
	case 0xCC: ABS(dis, dis_size, "CPY", addr, p_bytes); return;
	case 0xCD: ABS(dis, dis_size, "CMP", addr, p_bytes); return;
	case 0xCE: ABS(dis, dis_size, "DEC", addr, p_bytes); return;

	case 0xD0: BRX(dis, dis_size, "BNE", addr, p_conditional, p_addr2, p_bytes); return;
	case 0xD1: IndY(dis, dis_size, "CMP", addr, p_bytes); return;
	case 0xD5: ZPX(dis, dis_size, "CMP", addr, p_bytes); return;
	case 0xD6: ZPX(dis, dis_size, "DEC", addr, p_bytes); return;
	case 0xD8: strcpy_s(dis, dis_size, "CLD"); return;
	case 0xD9: ABSY(dis, dis_size, "CMP", addr, p_bytes); return;
	case 0xDD: ABSX(dis, dis_size, "CMP", addr, p_bytes); return;
	case 0xDE: ABSX(dis, dis_size, "DEC", addr, p_bytes); return;

	case 0xE0: IM(dis, dis_size, "CPX", addr, p_bytes); return;
	case 0xE1: IndX(dis, dis_size, "SBC", addr, p_bytes); return;
	case 0xE4: ZP(dis, dis_size, "CPX", addr, p_bytes); return;
	case 0xE5: ZP(dis, dis_size, "SBC", addr, p_bytes); return;
	case 0xE6: ZP(dis, dis_size, "INC", addr, p_bytes); return;
	case 0xE8: strcpy_s(dis, dis_size, "INX"); return;
	case 0xE9: IM(dis, dis_size, "SBC", addr, p_bytes); return;
	case 0xEA: strcpy_s(dis, dis_size, "NOP"); return;
	case 0xEC: ABS(dis, dis_size, "CPX", addr, p_bytes); return;
	case 0xED: ABS(dis, dis_size, "SBC", addr, p_bytes); return;
	case 0xEE: ABS(dis, dis_size, "INC", addr, p_bytes); return;

	case 0xF0: BRX(dis, dis_size, "BEQ", addr, p_conditional, p_addr2, p_bytes); return;
	case 0xF1: IndY(dis, dis_size, "SBC", addr, p_bytes); return;
	case 0xF5: ZPX(dis, dis_size, "SBC", addr, p_bytes); return;
	case 0xF6: ZPX(dis, dis_size, "INC", addr, p_bytes); return;
	case 0xF8: strcpy_s(dis, dis_size, "SED"); return;
	case 0xF9: ABSY(dis, dis_size, "SBC", addr, p_bytes); return;
	case 0xFD: ABSX(dis, dis_size, "SBC", addr, p_bytes); return;
	case 0xFE: ABSX(dis, dis_size, "INC", addr, p_bytes); return;

	default:
		strcpy_s(dis, dis_size, "???");
		return;
		//throw new Exception(string.Format("Invalid opcode {0:X2}", memory[addr]));
	}
}
