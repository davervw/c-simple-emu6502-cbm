// emu6502.c - Emu6502 - MOS6502 Emulator
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "emu6502.h"

extern byte A = 0;
extern byte X = 0;
extern byte Y = 0;
extern byte S = 0xFF;
extern bool N = false;
extern bool V = false;
extern bool B = false;
extern bool D = false;
extern bool I = false;
extern bool Z = false;
extern bool C = false;
extern ushort PC = 0;

extern bool trace = false;
extern bool step = false;

extern void ResetRun(bool (*ExecutePatch)(void))
{
	ushort addr = (ushort)((GetMemory(0xFFFC) | (GetMemory(0xFFFD) << 8))); // RESET vector
	Execute(addr, ExecutePatch);
}

static void PHP()
{
	int flags = (N ? 0x80 : 0)
		| (V ? 0x40 : 0)
		| (B ? 0x10 : 0)
		| (D ? 0x08 : 0)
		| (I ? 0x04 : 0)
		| (Z ? 0x02 : 0)
		| (C ? 0x01 : 0);
	Push(flags);
}

extern byte LO(ushort value)
{
	return (byte)value;
}

extern byte HI(ushort value)
{
	return (byte)(value >> 8);
}

static void BRK(byte *p_bytes)
{
	++PC;
	PHP();
	Push(HI(PC));
	Push(LO(PC));
	B = true;
	PC = (ushort)(GetMemory(0xFFFE) + (GetMemory(0xFFFF) << 8));
	*p_bytes = 0;
}

static byte Subtract(byte reg, byte value, bool *p_overflow)
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

static byte SubtractWithoutOverflow(byte reg, byte value)
{
	C = true; // init for CMP, etc.
	bool unused;
	return Subtract(reg, value, &unused);
}

static void CMP(byte value)
{
	SubtractWithoutOverflow(A, value);
}

static void CPX(byte value)
{
	SubtractWithoutOverflow(X, value);
}

static void CPY(byte value)
{
	SubtractWithoutOverflow(Y, value);
}

static void SetReg(byte *p_reg, int value)
{
	*p_reg = (byte)value;
	Z = (*p_reg == 0);
	N = ((*p_reg & 0x80) != 0);
}

static void SetA(int value)
{
	SetReg(&A, value);
}

static void SetX(int value)
{
	SetReg(&X, value);
}

static void SetY(int value)
{
	SetReg(&Y, value);
}

static void SBC(byte value)
{
	if (D)
	{
		int A_dec = (A & 0xF) + ((A >> 4) * 10);
		int value_dec = (value & 0xF) + ((value >> 4) * 10);
		int result_dec = A_dec - value_dec - (C ? 0 : 1);
		C = (result_dec >= 0);
		if (!C)
			result_dec = -result_dec; // absolute value
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

static void ADC(byte value)
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

static void ORA(int value)
{
	SetA(A | value);
}

static void EOR(int value)
{
	SetA(A ^ value);
}

static void AND(int value)
{
	SetA(A & value);
}

static void BIT(byte value)
{
	Z = (A & value) == 0;
	N = (value & 0x80) != 0;
	V = (value & 0x40) != 0;
}

static byte ASL(int value)
{
	C = (value & 0x80) != 0;
	value = (byte)(value << 1);
	Z = (value == 0);
	N = (value & 0x80) != 0;
	return (byte)value;
}

static byte LSR(int value)
{
	C = (value & 0x01) != 0;
	value = (byte)(value >> 1);
	Z = (value == 0);
	N = false;
	return (byte)value;
}

static byte ROL(int value)
{
	bool newC = (value & 0x80) != 0;
	value = (byte)((value << 1) | (C ? 1 : 0));
	C = newC;
	Z = (value == 0);
	N = (value & 0x80) != 0;
	return (byte)value;
}

static byte ROR(int value)
{
	bool newC = (value & 0x01) != 0;
	N = C;
	value = (byte)((value >> 1) | (C ? 0x80 : 0));
	C = newC;
	Z = (value == 0);
	return (byte)value;
}

extern void Push(int value)
{
	SetMemory((ushort)(0x100 + (S--)), (byte)value);
}

extern byte Pop(void)
{
	return GetMemory((ushort)(0x100 + (++S)));
}

static void PLP()
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

static void PHA()
{
	Push(A);
}

static void PLA()
{
	SetA(Pop());
}

static void CLC()
{
	C = false;
}

static void CLD()
{
	D = false;
}

static void CLI()
{
	I = false;
}

static void CLV()
{
	V = false;
}

static void SEC()
{
	C = true;
}

static void SED()
{
	D = true;
}

static void SEI()
{
	I = true;
}

static byte INC(byte value)
{
	++value;
	Z = (value == 0);
	N = (value & 0x80) != 0;
	return (byte)value;
}

static void INX()
{
	X = INC(X);
}

static void INY()
{
	Y = INC(Y);
}

static byte DEC(byte value)
{
	--value;
	Z = (value == 0);
	N = (value & 0x80) != 0;
	return (byte)value;
}

static void DEX()
{
	X = DEC(X);
}

static void DEY()
{
	Y = DEC(Y);
}

static void NOP()
{
}

static void TXA()
{
	SetReg(&A, X);
}

static void TAX()
{
	SetReg(&X, A);
}

static void TYA()
{
	SetReg(&A, Y);
}

static void TAY()
{
	SetReg(&Y, A);
}

static void TXS()
{
	S = X;
}

static void TSX()
{
	SetReg(&X, S);
}

static ushort GetBR(ushort addr, bool *p_conditional, byte *p_bytes)
{
	*p_conditional = true;
	*p_bytes = 2;
	sbyte offset = (sbyte)GetMemory((ushort)(addr + 1));
	ushort addr2 = (ushort)(addr + 2 + offset);
	return addr2;
}

static void BR(bool branch, ushort *p_addr, bool *p_conditional, byte *p_bytes)
{
	ushort addr2 = GetBR(*p_addr, p_conditional, p_bytes);
	if (branch)
	{
		*p_addr = addr2;
		*p_bytes = 0; // don't advance addr
	}
}

static void BPL(ushort *p_addr, bool *p_conditional, byte *p_bytes)
{
	BR(!N, p_addr, p_conditional, p_bytes);
}

static void BMI(ushort *p_addr, bool *p_conditional, byte *p_bytes)
{
	BR(N, p_addr, p_conditional, p_bytes);
}

static void BCC(ushort *p_addr, bool *p_conditional, byte *p_bytes)
{
	BR(!C, p_addr, p_conditional, p_bytes);
}

static void BCS(ushort *p_addr, bool *p_conditional, byte *p_bytes)
{
	BR(C, p_addr, p_conditional, p_bytes);
}

static void BVC(ushort *p_addr, bool *p_conditional, byte *p_bytes)
{
	BR(!V, p_addr, p_conditional, p_bytes);
}

static void BVS(ushort *p_addr, bool *p_conditional, byte *p_bytes)
{
	BR(V, p_addr, p_conditional, p_bytes);
}

static void BNE(ushort *p_addr, bool *p_conditional, byte *p_bytes)
{
	BR(!Z, p_addr, p_conditional, p_bytes);
}

static void BEQ(ushort *p_addr, bool *p_conditional, byte *p_bytes)
{
	BR(Z, p_addr, p_conditional, p_bytes);
}

static void JSR(ushort *p_addr, byte *p_bytes)
{
	*p_bytes = 3; // for next calculation
	ushort addr2 = (ushort)(*p_addr + *p_bytes - 1);
	ushort addr3 = (ushort)(GetMemory((ushort)(*p_addr + 1)) | (GetMemory((ushort)(*p_addr + 2)) << 8));
	Push(HI(addr2));
	Push(LO(addr2));
	*p_addr = addr3;
	*p_bytes = 0; // addr already changed
}

static void RTS(ushort *p_addr, byte *p_bytes)
{
	byte lo = Pop();
	byte hi = Pop();
	*p_bytes = 1; // make sure caller increases addr by one
	*p_addr = (ushort)((hi << 8) | lo);
}

static void RTI(ushort *p_addr, byte *p_bytes)
{
	PLP();
	byte hi = Pop();
	byte lo = Pop();
	*p_bytes = 0; // make sure caller does not increase addr by one
	*p_addr = (ushort)((hi << 8) | lo);
}

static void JMP(ushort *p_addr, byte *p_bytes)
{
	*p_bytes = 0; // caller should not advance address
	ushort addr2 = (ushort)(GetMemory((ushort)(*p_addr + 1)) | (GetMemory((ushort)(*p_addr + 2)) << 8));
	*p_addr = addr2;
}

static void JMPIND(ushort *p_addr, byte *p_bytes)
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
static void GetDisplayState(char state[32])
{
	sprintf_s(state, sizeof(state), "A:%X2 X:%X2 Y:%X2 S:%X2 P:%c%c-%c%c%c%c%c",
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

static byte GetIndX(ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = (ushort)(GetMemory((ushort)(addr + 1)) + X);
	return GetMemory((ushort)(GetMemory(addr2) | (GetMemory((ushort)(addr2 + 1)) << 8)));
}

static void SetIndX(byte value, ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = (ushort)(GetMemory((ushort)(addr + 1)) + X);
	ushort addr3 = (ushort)(GetMemory(addr2) | (GetMemory((ushort)(addr2 + 1)) << 8));
	SetMemory(addr3, value);
}

static byte GetIndY(ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = (ushort)(GetMemory((ushort)(addr + 1)));
	ushort addr3 = (ushort)((GetMemory(addr2) | (GetMemory((ushort)(addr2 + 1)) << 8)) + Y);
	return GetMemory(addr3);
}

static void SetIndY(byte value, ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = (ushort)(GetMemory((ushort)(addr + 1)));
	ushort addr3 = (ushort)((GetMemory(addr2) | (GetMemory((ushort)(addr2 + 1)) << 8)) + Y);
	SetMemory(addr3, value);
}

static byte GetZP(ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = GetMemory((ushort)(addr + 1));
	return GetMemory(addr2);
}

static void SetZP(byte value, ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = GetMemory((ushort)(addr + 1));
	SetMemory(addr2, value);
}

static byte GetZPX(ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = GetMemory((ushort)(addr + 1));
	return GetMemory((byte)(addr2 + X));
}

static void SetZPX(byte value, ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = GetMemory((ushort)(addr + 1));
	SetMemory((byte)(addr2 + X), value);
}

static byte GetZPY(ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = GetMemory((ushort)(addr + 1));
	return GetMemory((byte)(addr2 + Y));
}

static void SetZPY(byte value, ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = GetMemory((ushort)(addr + 1));
	SetMemory((byte)(addr2 + Y), value);
}

static byte GetABS(ushort addr, byte *p_bytes)
{
	*p_bytes = 3;
	ushort addr2 = (ushort)(GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
	return GetMemory(addr2);
}

static void SetABS(byte value, ushort addr, byte *p_bytes)
{
	*p_bytes = 3;
	ushort addr2 = (ushort)(GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
	SetMemory(addr2, value);
}

static byte GetABSX(ushort addr, byte *p_bytes)
{
	*p_bytes = 3;
	ushort addr2 = (ushort)(GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
	return GetMemory((ushort)(addr2 + X));
}

static void SetABSX(byte value, ushort addr, byte *p_bytes)
{
	*p_bytes = 3;
	ushort addr2 = (ushort)((GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8)) + X);
	SetMemory(addr2, value);
}

static byte GetABSY(ushort addr, byte *p_bytes)
{
	*p_bytes = 3;
	ushort addr2 = (ushort)(GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
	return GetMemory((ushort)(addr2 + Y));
}

static void SetABSY(byte value, ushort addr, byte *p_bytes)
{
	*p_bytes = 3;
	ushort addr2 = (ushort)((GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8)) + Y);
	SetMemory(addr2, value);
}

static byte GetIM(ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	return GetMemory((ushort)(addr + 1));
}

extern void Execute(ushort addr, bool (*ExecutePatch)(void))
{
	bool conditional;
	byte bytes;

	PC = addr;

	while (true)
	{
		while (true)
		{
			bytes = 1;
			//bool breakpoint = false;
			//if (Breakpoints.Contains(PC))
			//	breakpoint = true;
			if (trace /*|| breakpoint*/ || step)
			{
				ushort addr2;
				char line[26];
				char dis[12];
				DisassembleLong(PC, &conditional, &bytes, &addr2, line, dis);
				char state[32];
				GetDisplayState(state);
				fprintf(stderr, "%s%-30s", line, state);
				if (step)
					step = step; // user can put debug breakpoint here to allow stepping
				//if (breakpoint)
				//	breakpoint = breakpoint; // user can put debug breakpoint here to allow break
			}
			if (ExecutePatch != 0 && !ExecutePatch()) // allow execute to be overriden at a specific address
				break;
		}

		switch (GetMemory(PC))
		{
		case 0x00: BRK(&bytes); break;
		case 0x01: ORA(GetIndX(PC, &bytes)); break;
		case 0x05: ORA(GetZP(PC, &bytes)); break;
		case 0x06: SetZP(ASL(GetZP(PC, &bytes)), PC, &bytes); break;
		case 0x08: PHP(); break;
		case 0x09: ORA(GetIM(PC, &bytes)); break;
		case 0x0A: SetA(ASL(A)); break;
		case 0x0D: ORA(GetABS(PC, &bytes)); break;
		case 0x0E: SetABS(ASL(GetABS(PC, &bytes)), PC, &bytes); break;

		case 0x10: BPL(&PC, &conditional, &bytes); break;
		case 0x11: ORA(GetIndY(PC, &bytes)); break;
		case 0x15: ORA(GetZPX(PC, &bytes)); break;
		case 0x16: SetZPX(ASL(GetZPX(PC, &bytes)), PC, &bytes); break;
		case 0x18: CLC(); break;
		case 0x19: ORA(GetABSY(PC, &bytes)); break;
		case 0x1D: ORA(GetABSX(PC, &bytes)); break;
		case 0x1E: SetABSX(ASL(GetABSX(PC, &bytes)), PC, &bytes); break;

		case 0x20: JSR(&PC, &bytes); break;
		case 0x21: AND(GetIndX(PC, &bytes)); break;
		case 0x24: BIT(GetZP(PC, &bytes)); break;
		case 0x25: AND(GetZP(PC, &bytes)); break;
		case 0x26: SetZP(ROL(GetZP(PC, &bytes)), PC, &bytes); break;
		case 0x28: PLP(); break;
		case 0x29: AND(GetIM(PC, &bytes)); break;
		case 0x2A: SetA(ROL(A)); break;
		case 0x2C: BIT(GetABS(PC, &bytes)); break;
		case 0x2D: AND(GetABS(PC, &bytes)); break;
		case 0x2E: ROL(GetABS(PC, &bytes)); break;

		case 0x30: BMI(&PC, &conditional, &bytes); break;
		case 0x31: AND(GetIndY(PC, &bytes)); break;
		case 0x35: AND(GetZPX(PC, &bytes)); break;
		case 0x36: SetZPX(ROL(GetZPX(PC, &bytes)), PC, &bytes); break;
		case 0x38: SEC(); break;
		case 0x39: AND(GetABSY(PC, &bytes)); break;
		case 0x3D: AND(GetABSX(PC, &bytes)); break;
		case 0x3E: SetABSX(ROL(GetABSX(PC, &bytes)), PC, &bytes); break;

		case 0x40: RTI(&PC, &bytes); break;
		case 0x41: EOR(GetIndX(PC, &bytes)); break;
		case 0x45: EOR(GetZP(PC, &bytes)); break;
		case 0x46: SetZP(LSR(GetZP(PC, &bytes)), PC, &bytes); break;
		case 0x48: PHA(); break;
		case 0x49: EOR(GetIM(PC, &bytes)); break;
		case 0x4A: SetA(LSR(A)); break;
		case 0x4C: JMP(&PC, &bytes); break;
		case 0x4D: EOR(GetABS(PC, &bytes)); break;
		case 0x4E: LSR(GetABS(PC, &bytes)); break;

		case 0x50: BVC(&PC, &conditional, &bytes); break;
		case 0x51: EOR(GetIndY(PC, &bytes)); break;
		case 0x55: EOR(GetZPX(PC, &bytes)); break;
		case 0x56: SetZPX(LSR(GetZPX(PC, &bytes)), PC, &bytes); break;
		case 0x58: CLI(); break;
		case 0x59: EOR(GetABSY(PC, &bytes)); break;
		case 0x5D: EOR(GetABSX(PC, &bytes)); break;
		case 0x5E: SetABSX(LSR(GetABSX(PC, &bytes)), PC, &bytes); break;

		case 0x60: RTS(&PC, &bytes); break;
		case 0x61: ADC(GetIndX(PC, &bytes)); break;
		case 0x65: ADC(GetZP(PC, &bytes)); break;
		case 0x66: SetZP(ROR(GetZP(PC, &bytes)), PC, &bytes); break;
		case 0x68: PLA(); break;
		case 0x69: ADC(GetIM(PC, &bytes)); break;
		case 0x6A: SetA(ROR(A)); break;
		case 0x6C: JMPIND(&PC, &bytes); break;
		case 0x6D: ADC(GetABS(PC, &bytes)); break;
		case 0x6E: SetABS(ROR(GetABS(PC, &bytes)), PC, &bytes); break;

		case 0x70: BVS(&PC, &conditional, &bytes); break;
		case 0x71: ADC(GetIndY(PC, &bytes)); break;
		case 0x75: ADC(GetZPX(PC, &bytes)); break;
		case 0x76: SetZPX(ROR(GetZPX(PC, &bytes)), PC, &bytes); break;
		case 0x78: SEI(); break;
		case 0x79: ADC(GetABSY(PC, &bytes)); break;
		case 0x7D: ADC(GetABSX(PC, &bytes)); break;
		case 0x7E: SetABSX(ROR(GetABSX(PC, &bytes)), PC, &bytes); break;

		case 0x81: SetIndX(A, PC, &bytes); break;
		case 0x84: SetZP(Y, PC, &bytes); break;
		case 0x85: SetZP(A, PC, &bytes); break;
		case 0x86: SetZP(X, PC, &bytes); break;
		case 0x88: DEY(); break;
		case 0x8A: TXA(); break;
		case 0x8C: SetABS(Y, PC, &bytes); break;
		case 0x8D: SetABS(A, PC, &bytes); break;
		case 0x8E: SetABS(X, PC, &bytes); break;

		case 0x90: BCC(&PC, &conditional, &bytes); break;
		case 0x91: SetIndY(A, PC, &bytes); break;
		case 0x94: SetZPX(Y, PC, &bytes); break;
		case 0x95: SetZPX(A, PC, &bytes); break;
		case 0x96: SetZPY(X, PC, &bytes); break;
		case 0x98: TYA(); break;
		case 0x99: SetABSY(A, PC, &bytes); break;
		case 0x9A: TXS(); break;
		case 0x9D: SetABSX(A, PC, &bytes); break;

		case 0xA0: SetY(GetIM(PC, &bytes)); break;
		case 0xA1: SetA(GetIndX(PC, &bytes)); break;
		case 0xA2: SetX(GetIM(PC, &bytes)); break;
		case 0xA4: SetY(GetZP(PC, &bytes)); break;
		case 0xA5: SetA(GetZP(PC, &bytes)); break;
		case 0xA6: SetX(GetZP(PC, &bytes)); break;
		case 0xA8: TAY(); break;
		case 0xA9: SetA(GetIM(PC, &bytes)); break;
		case 0xAA: TAX(); break;
		case 0xAC: SetY(GetABS(PC, &bytes)); break;
		case 0xAD: SetA(GetABS(PC, &bytes)); break;
		case 0xAE: SetX(GetABS(PC, &bytes)); break;

		case 0xB0: BCS(&PC, &conditional, &bytes); break;
		case 0xB1: SetA(GetIndY(PC, &bytes)); break;
		case 0xB4: SetY(GetZPX(PC, &bytes)); break;
		case 0xB5: SetA(GetZPX(PC, &bytes)); break;
		case 0xB6: SetX(GetZPY(PC, &bytes)); break;
		case 0xB8: CLV(); break;
		case 0xB9: SetA(GetABSY(PC, &bytes)); break;
		case 0xBA: TSX(); break;
		case 0xBC: SetY(GetABSX(PC, &bytes)); break;
		case 0xBD: SetA(GetABSX(PC, &bytes)); break;
		case 0xBE: SetX(GetABSY(PC, &bytes)); break;

		case 0xC0: CPY(GetIM(PC, &bytes)); break;
		case 0xC1: CMP(GetIndX(PC, &bytes)); break;
		case 0xC4: CPY(GetZP(PC, &bytes)); break;
		case 0xC5: CMP(GetZP(PC, &bytes)); break;
		case 0xC6: SetZP(DEC(GetZP(PC, &bytes)), PC, &bytes); break;
		case 0xC8: INY(); break;
		case 0xC9: CMP(GetIM(PC, &bytes)); break;
		case 0xCA: DEX(); break;
		case 0xCC: CPY(GetABS(PC, &bytes)); break;
		case 0xCD: CMP(GetABS(PC, &bytes)); break;
		case 0xCE: SetABS(DEC(GetABS(PC, &bytes)), PC, &bytes); break;

		case 0xD0: BNE(&PC, &conditional, &bytes); break;
		case 0xD1: CMP(GetIndY(PC, &bytes)); break;
		case 0xD5: CMP(GetZPX(PC, &bytes)); break;
		case 0xD6: SetZPX(DEC(GetZPX(PC, &bytes)), PC, &bytes); break;
		case 0xD8: CLD(); break;
		case 0xD9: CMP(GetABSY(PC, &bytes)); break;
		case 0xDD: CMP(GetABSX(PC, &bytes)); break;
		case 0xDE: SetABSX(DEC(GetABSX(PC, &bytes)), PC, &bytes); break;

		case 0xE0: CPX(GetIM(PC, &bytes)); break;
		case 0xE1: SBC(GetIndX(PC, &bytes)); break;
		case 0xE4: CPX(GetZP(PC, &bytes)); break;
		case 0xE5: SBC(GetZP(PC, &bytes)); break;
		case 0xE6: SetZP(INC(GetZP(PC, &bytes)), PC, &bytes); break;
		case 0xE8: INX(); break;
		case 0xE9: SBC(GetIM(PC, &bytes)); break;
		case 0xEA: NOP(); break;
		case 0xEC: CPX(GetABS(PC, &bytes)); break;
		case 0xED: SBC(GetABS(PC, &bytes)); break;
		case 0xEE: SetABS(INC(GetABS(PC, &bytes)), PC, &bytes); break;

		case 0xF0: BEQ(&PC, &conditional, &bytes); break;
		case 0xF1: SBC(GetIndY(PC, &bytes)); break;
		case 0xF5: SBC(GetZPX(PC, &bytes)); break;
		case 0xF6: SetZPX(INC(GetZPX(PC, &bytes)), PC, &bytes); break;
		case 0xF8: SED(); break;
		case 0xF9: SBC(GetABSY(PC, &bytes)); break;
		case 0xFD: SBC(GetABSX(PC, &bytes)); break;
		case 0xFE: SetABSX(INC(GetABSX(PC, &bytes)), PC, &bytes); break;

		default:
			{
				printf("Invalid opcode %02X at %04X", GetMemory(PC), PC);
				exit(1);
			}
		}

		PC += bytes;
	}
}

// Examples:
// FFFF FF FF FF JMP ($FFFF)
// FFFF FF FF FF LDA $FFFF,X
extern void DisassembleLong(ushort addr, bool *p_conditional, byte *p_bytes, ushort *p_addr2, byte dis[12], byte line[26])
{
	DisassembleShort(addr, p_conditional, p_bytes, p_addr2, dis);
	sprintf_s(line, strlen(line), "%04X", addr);
	for (int i = 0; i < 3; ++i)
	{
		if (i < *p_bytes)
			sprintf_s(line+strlen(line), sizeof(line)-strlen(line), "%02X", GetMemory((ushort)(addr + i)));
		else
			strcat_s(line, sizeof(line), "   ");
	}
	strcat_s(line, sizeof(line), dis);
}

static void Ind(char dis[12], char* opcode, ushort addr, ushort *p_addr2, byte *p_bytes)
{
	*p_bytes = 3;
	ushort addr1 = (ushort)(GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
	*p_addr2 = (ushort)(GetMemory(addr1) | (GetMemory((ushort)(addr1 + 1)) << 8));
	sprintf_s(dis, sizeof(dis), "%s ($%0X4)", opcode, addr1);
}

static void IndX(char dis[12], char* opcode, ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	sprintf_s(dis, sizeof(dis), "%s ($%0X2,X)", opcode, GetMemory((ushort)(addr + 1)));
}

static void IndY(char dis[12], char* opcode, ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	sprintf_s(dis, sizeof(dis), "%s ($%0X2),Y", opcode, GetMemory((ushort)(addr + 1)));
}

static void ZP(char dis[12], char* opcode, ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	sprintf_s(dis, sizeof(dis), "%s $%0X2", opcode, GetMemory((ushort)(addr + 1)));
}

static void ZPX(char dis[12], char* opcode, ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	sprintf_s(dis, sizeof(dis), "%s $%0X2,X", opcode, GetMemory((ushort)(addr + 1)));
}

static void ZPY(char dis[12], char* opcode, ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	sprintf_s(dis, sizeof(dis), "%s $%0X2,Y", opcode, GetMemory((ushort)(addr + 1)));
}

static void ABS(byte dis[12], char* opcode, ushort addr, byte *p_bytes)
{
	*p_bytes = 3;
	sprintf_s(dis, sizeof(dis), "%s $%04X", opcode, GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
}

static void ABSAddr(char dis[12], char* opcode, ushort addr, ushort *p_addr2, byte *p_bytes)
{
	*p_bytes = 3;
	*p_addr2 = (ushort)(GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
	sprintf_s(dis, sizeof(dis), "%s $%04X", opcode, *p_addr2);
}

static void ABSX(char dis[12], char* opcode, ushort addr, byte *p_bytes)
{
	*p_bytes = 3;
	sprintf_s(dis, sizeof(dis), "%s $%04X,X", opcode, GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
}

static void ABSY(char dis[12], char* opcode, ushort addr, byte *p_bytes)
{
	*p_bytes = 3;
	sprintf_s(dis, sizeof(dis), "%s $%04X,Y", opcode, GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
}

static void IM(char dis[12], char* opcode, ushort addr, byte *p_bytes)
{
	*p_bytes = 2;
	sprintf_s(dis, sizeof(dis), "%s #$%0X2", opcode, GetMemory((ushort)(addr + 1)));
}

static void BRX(char dis[12], char* opcode, ushort addr, bool *p_conditional, ushort *p_addr2, byte *p_bytes)
{
	*p_bytes = 2;
	*p_conditional = true;
	sbyte offset = (sbyte)GetMemory((ushort)(addr + 1));
	*p_addr2 = (ushort)(addr + 2 + offset);
	sprintf_s(dis, sizeof(dis), "%s $%04X", opcode, *p_addr2);
}

// JMP ($FFFF)
// LDA $FFFF,X
extern void DisassembleShort(ushort addr, bool *p_conditional, byte *p_bytes, ushort *p_addr2, byte dis[12])
{
	*p_conditional = false;
	*p_addr2 = 0;
	*p_bytes = 1;

	switch (GetMemory(addr))
	{
	case 0x00: strcpy_s(dis, sizeof(dis), "BRK"); return;
	case 0x01: IndX(dis, "ORA", addr, p_bytes); return;
	case 0x05: ZP(dis, "ORA", addr, p_bytes); return;
	case 0x06: ZP(dis, "ASL", addr, p_bytes); return;
	case 0x08: strcpy_s(dis, sizeof(dis), "PHP"); return;
	case 0x09: IM(dis, "ORA", addr, p_bytes); return;
	case 0x0A: strcpy_s(dis, sizeof(dis), "ASL A"); return;
	case 0x0D: ABS(dis, "ORA", addr, p_bytes); return;
	case 0x0E: ABS(dis, "ASL", addr, p_bytes); return;

	case 0x10: BRX(dis, "BPL", addr, p_conditional, p_addr2, p_bytes); return;
	case 0x11: IndY(dis, "ORA", addr, p_bytes); return;
	case 0x15: ZPX(dis, "ORA", addr, p_bytes); return;
	case 0x16: ZPX(dis, "ASL", addr, p_bytes); return;
	case 0x18: strcpy_s(dis, sizeof(dis), "CLC"); return;
	case 0x19: ABSY(dis, "ORA", addr, p_bytes); return;
	case 0x1D: ABSX(dis, "ORA", addr, p_bytes); return;
	case 0x1E: ABSX(dis, "ASL", addr, p_bytes); return;

	case 0x20: ABSAddr(dis, "JSR", addr, p_addr2, p_bytes); return;
	case 0x21: IndX(dis, "AND", addr, p_bytes); return;
	case 0x24: ZP(dis, "BIT", addr, p_bytes); return;
	case 0x25: ZP(dis, "AND", addr, p_bytes); return;
	case 0x26: ZP(dis, "ROL", addr, p_bytes); return;
	case 0x28: strcpy_s(dis, sizeof(dis), "PLP"); return;
	case 0x29: IM(dis, "AND", addr, p_bytes); return;
	case 0x2A: strcpy_s(dis, sizeof(dis), "ROL A"); return;
	case 0x2C: ABS(dis, "BIT", addr, p_bytes); return;
	case 0x2D: ABS(dis, "AND", addr, p_bytes); return;
	case 0x2E: ABS(dis, "ROL", addr, p_bytes); return;

	case 0x30: BRX(dis, "BMI", addr, p_conditional, p_addr2, p_bytes); return;
	case 0x31: IndY(dis, "AND", addr, p_bytes); return;
	case 0x35: ZPX(dis, "AND", addr, p_bytes); return;
	case 0x36: ZPX(dis, "ROL", addr, p_bytes); return;
	case 0x38: strcpy_s(dis, sizeof(dis), "SEC"); return;
	case 0x39: ABSY(dis, "AND", addr, p_bytes); return;
	case 0x3D: ABSX(dis, "AND", addr, p_bytes); return;
	case 0x3E: ABSX(dis, "ROL", addr, p_bytes); return;

	case 0x40: strcpy_s(dis, sizeof(dis), "RTI"); return;
	case 0x41: IndX(dis, "EOR", addr, p_bytes); return;
	case 0x45: ZP(dis, "EOR", addr, p_bytes); return;
	case 0x46: ZP(dis, "LSR", addr, p_bytes); return;
	case 0x48: strcpy_s(dis, sizeof(dis), "PHA"); return;
	case 0x49: IM(dis, "EOR", addr, p_bytes); return;
	case 0x4A: strcpy_s(dis, sizeof(dis), "LSR A"); return;
	case 0x4C: ABSAddr(dis, "JMP", addr, p_addr2, p_bytes); return;
	case 0x4D: ABS(dis, "EOR", addr, p_bytes); return;
	case 0x4E: ABS(dis, "LSR", addr, p_bytes); return;

	case 0x50: BRX(dis, "BVC", addr, p_conditional, p_addr2, p_bytes); return;
	case 0x51: IndY(dis, "EOR", addr, p_bytes); return;
	case 0x55: ZPX(dis, "EOR", addr, p_bytes); return;
	case 0x56: ZPX(dis, "LSR", addr, p_bytes); return;
	case 0x58: strcpy_s(dis, sizeof(dis), "CLI"); return;
	case 0x59: ABSY(dis, "EOR", addr, p_bytes); return;
	case 0x5D: ABSX(dis, "EOR", addr, p_bytes); return;
	case 0x5E: ABSX(dis, "LSR", addr, p_bytes); return;

	case 0x60: strcpy_s(dis, sizeof(dis), "RTS"); return;
	case 0x61: IndX(dis, "ADC", addr, p_bytes); return;
	case 0x65: ZP(dis, "ADC", addr, p_bytes); return;
	case 0x66: ZP(dis, "ROR", addr, p_bytes); return;
	case 0x68: strcpy_s(dis, sizeof(dis), "PLA"); return;
	case 0x69: IM(dis, "ADC", addr, p_bytes); return;
	case 0x6A: strcpy_s(dis, sizeof(dis), "ROR A"); return;
	case 0x6C: Ind(dis, "JMP", addr, p_addr2, p_bytes); return;
	case 0x6D: ABS(dis, "ADC", addr, p_bytes); return;
	case 0x6E: ABS(dis, "ROR", addr, p_bytes); return;

	case 0x70: BRX(dis, "BVS", addr, p_conditional, p_addr2, p_bytes); return;
	case 0x71: IndY(dis, "ADC", addr, p_bytes); return;
	case 0x75: ZPX(dis, "ADC", addr, p_bytes); return;
	case 0x76: ZPX(dis, "ROR", addr, p_bytes); return;
	case 0x78: strcpy_s(dis, sizeof(dis), "SEI"); return;
	case 0x79: ABSY(dis, "ADC", addr, p_bytes); return;
	case 0x7D: ABSX(dis, "ADC", addr, p_bytes); return;
	case 0x7E: ABSX(dis, "ROR", addr, p_bytes); return;

	case 0x81: IndX(dis, "STA", addr, p_bytes); return;
	case 0x84: ZP(dis, "STY", addr, p_bytes); return;
	case 0x85: ZP(dis, "STA", addr, p_bytes); return;
	case 0x86: ZP(dis, "STX", addr, p_bytes); return;
	case 0x88: strcpy_s(dis, sizeof(dis), "DEY"); return;
	case 0x8A: strcpy_s(dis, sizeof(dis), "TXA"); return;
	case 0x8C: ABS(dis, "STY", addr, p_bytes); return;
	case 0x8D: ABS(dis, "STA", addr, p_bytes); return;
	case 0x8E: ABS(dis, "STX", addr, p_bytes); return;

	case 0x90: BRX(dis, "BCC", addr, p_conditional, p_addr2, p_bytes); return;
	case 0x91: IndY(dis, "STA", addr, p_bytes); return;
	case 0x94: ZPX(dis, "STY", addr, p_bytes); return;
	case 0x95: ZPX(dis, "STA", addr, p_bytes); return;
	case 0x96: ZPY(dis, "STX", addr, p_bytes); return;
	case 0x98: strcpy_s(dis, sizeof(dis), "TYA"); return;
	case 0x99: ABSY(dis, "STA", addr, p_bytes); return;
	case 0x9A: strcpy_s(dis, sizeof(dis), "TXS"); return;
	case 0x9D: ABSX(dis, "STA", addr, p_bytes); return;

	case 0xA0: IM(dis, "LDY", addr, p_bytes); return;
	case 0xA1: IndX(dis, "LDA", addr, p_bytes); return;
	case 0xA2: IM(dis, "LDX", addr, p_bytes); return;
	case 0xA4: ZP(dis, "LDY", addr, p_bytes); return;
	case 0xA5: ZP(dis, "LDA", addr, p_bytes); return;
	case 0xA6: ZP(dis, "LDX", addr, p_bytes); return;
	case 0xA8: strcpy_s(dis, sizeof(dis), "TAY"); return;
	case 0xA9: IM(dis, "LDA", addr, p_bytes); return;
	case 0xAA: strcpy_s(dis, sizeof(dis), "TAX"); return;
	case 0xAC: ABS(dis, "LDY", addr, p_bytes); return;
	case 0xAD: ABS(dis, "LDA", addr, p_bytes); return;
	case 0xAE: ABS(dis, "LDX", addr, p_bytes); return;

	case 0xB0: BRX(dis, "BCS", addr, p_conditional, p_addr2, p_bytes); return;
	case 0xB1: IndY(dis, "LDA", addr, p_bytes); return;
	case 0xB4: ZPX(dis, "LDY", addr, p_bytes); return;
	case 0xB5: ZPX(dis, "LDA", addr, p_bytes); return;
	case 0xB6: ZPY(dis, "LDX", addr, p_bytes); return;
	case 0xB8: strcpy_s(dis, sizeof(dis), "CLV"); return;
	case 0xB9: ABSY(dis, "LDA", addr, p_bytes); return;
	case 0xBA: strcpy_s(dis, sizeof(dis), "TSX"); return;
	case 0xBC: ABSX(dis, "LDY", addr, p_bytes); return;
	case 0xBD: ABSX(dis, "LDA", addr, p_bytes); return;
	case 0xBE: ABSY(dis, "LDX", addr, p_bytes); return;

	case 0xC0: IM(dis, "CPY", addr, p_bytes); return;
	case 0xC1: IndX(dis, "CMP", addr, p_bytes); return;
	case 0xC4: ZP(dis, "CPY", addr, p_bytes); return;
	case 0xC5: ZP(dis, "CMP", addr, p_bytes); return;
	case 0xC6: ZP(dis, "DEC", addr, p_bytes); return;
	case 0xC8: strcpy_s(dis, sizeof(dis), "INY"); return;
	case 0xC9: IM(dis, "CMP", addr, p_bytes); return;
	case 0xCA: strcpy_s(dis, sizeof(dis), "DEX"); return;
	case 0xCC: ABS(dis, "CPY", addr, p_bytes); return;
	case 0xCD: ABS(dis, "CMP", addr, p_bytes); return;
	case 0xCE: ABS(dis, "DEC", addr, p_bytes); return;

	case 0xD0: BRX(dis, "BNE", addr, p_conditional, p_addr2, p_bytes); return;
	case 0xD1: IndY(dis, "CMP", addr, p_bytes); return;
	case 0xD5: ZPX(dis, "CMP", addr, p_bytes); return;
	case 0xD6: ZPX(dis, "DEC", addr, p_bytes); return;
	case 0xD8: strcpy_s(dis, sizeof(dis), "CLD"); return;
	case 0xD9: ABSY(dis, "CMP", addr, p_bytes); return;
	case 0xDD: ABSX(dis, "CMP", addr, p_bytes); return;
	case 0xDE: ABSX(dis, "DEC", addr, p_bytes); return;

	case 0xE0: IM(dis, "CPX", addr, p_bytes); return;
	case 0xE1: IndX(dis, "SBC", addr, p_bytes); return;
	case 0xE4: ZP(dis, "CPX", addr, p_bytes); return;
	case 0xE5: ZP(dis, "SBC", addr, p_bytes); return;
	case 0xE6: ZP(dis, "INC", addr, p_bytes); return;
	case 0xE8: strcpy_s(dis, sizeof(dis), "INX"); return;
	case 0xE9: IM(dis, "SBC", addr, p_bytes); return;
	case 0xEA: strcpy_s(dis, sizeof(dis), "NOP"); return;
	case 0xEC: ABS(dis, "CPX", addr, p_bytes); return;
	case 0xED: ABS(dis, "SBC", addr, p_bytes); return;
	case 0xEE: ABS(dis, "INC", addr, p_bytes); return;

	case 0xF0: BRX(dis, "BEQ", addr, p_conditional, p_addr2, p_bytes); return;
	case 0xF1: IndY(dis, "SBC", addr, p_bytes); return;
	case 0xF5: ZPX(dis, "SBC", addr, p_bytes); return;
	case 0xF6: ZPX(dis, "INC", addr, p_bytes); return;
	case 0xF8: strcpy_s(dis, sizeof(dis), "SED"); return;
	case 0xF9: ABSY(dis, "SBC", addr, p_bytes); return;
	case 0xFD: ABSX(dis, "SBC", addr, p_bytes); return;
	case 0xFE: ABSX(dis, "INC", addr, p_bytes); return;

	default:
		strcpy_s(dis, sizeof(dis), "???");
		return;
		//throw new Exception(string.Format("Invalid opcode {0:X2}", memory[addr]));
	}
}
