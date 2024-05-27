// emu6502.c - Emu6502 - MOS6502 Emulator
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C Portable Version)
// C64/6502 Unified Emulator for M5Stack/Teensy/ESP32 LCDs and Windows
//
// MIT License
//
// Copyright (c) 2024 by David R. Van Wagner
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

#include "emu6502.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WINDOWS
#include <Windows.h>
#include <WindowsTime.h>
#else
void strcpy_s(char* dest, size_t size, const char* src) { strncpy(dest, src, size); }
void strcat_s(char* dest, size_t size, const char* src) { strncat(dest, src, size); }
#endif

Emu6502::Emu6502(Memory* mem)
{
	memory = mem;
	A = 0;
	X = 0;
	Y = 0;
	S = 0xFF;
	N = false;
	V = false;
	B = true; // 6502 PHP instructions always treats B flag as true, so keep it true
	D = false;
	I = false;
	Z = false;
	C = false;
	PC = 0;
	trace = false;
	step = false;
	quit = false;
	sixty_hz_irq = true;
	timer_now = micros();
}

Emu6502::~Emu6502()
{
}

void Emu6502::ResetRun()
{
	ushort addr = GetMemoryWord(0xFFFC); // RESET vector
	Execute(addr);
}

void Emu6502::PHP()
{
	int flags = (N ? 0x80 : 0)
		| (V ? 0x40 : 0)
		| 0x20 // reserved, always set when pushed
		| 0x10 // B(RK) always set when pushed
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

byte Emu6502::Subtract(byte reg, byte value, bool* p_overflow)
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

void Emu6502::SetReg(byte* p_reg, int value)
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
			result_dec += 100; // adjust negative result
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

void Emu6502::ADDC(byte value)
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

void Emu6502::BITOP(byte value)
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

byte Emu6502::DECR(byte value)
{
	--value;
	Z = (value == 0);
	N = (value & 0x80) != 0;
	return (byte)value;
}

void Emu6502::DEX()
{
	X = DECR(X);
}

void Emu6502::DEY()
{
	Y = DECR(Y);
}

void Emu6502::NO_OP()
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

ushort Emu6502::GetBR(ushort addr, bool* p_conditional, byte* p_bytes)
{
	*p_conditional = true;
	*p_bytes = 2;
	sbyte offset = (sbyte)GetMemory((ushort)(addr + 1));
	ushort addr2 = (ushort)(addr + 2 + offset);
	return addr2;
}

void Emu6502::BRANCH(bool branch, ushort* p_addr, bool* p_conditional, byte* p_bytes)
{
	ushort addr2 = GetBR(*p_addr, p_conditional, p_bytes);
	if (branch)
	{
		*p_addr = addr2;
		*p_bytes = 0; // don't advance addr
	}
}

void Emu6502::BPL(ushort* p_addr, bool* p_conditional, byte* p_bytes)
{
	BRANCH(!N, p_addr, p_conditional, p_bytes);
}

void Emu6502::BMI(ushort* p_addr, bool* p_conditional, byte* p_bytes)
{
	BRANCH(N, p_addr, p_conditional, p_bytes);
}

void Emu6502::BCC(ushort* p_addr, bool* p_conditional, byte* p_bytes)
{
	BRANCH(!C, p_addr, p_conditional, p_bytes);
}

void Emu6502::BCS(ushort* p_addr, bool* p_conditional, byte* p_bytes)
{
	BRANCH(C, p_addr, p_conditional, p_bytes);
}

void Emu6502::BVC(ushort* p_addr, bool* p_conditional, byte* p_bytes)
{
	BRANCH(!V, p_addr, p_conditional, p_bytes);
}

void Emu6502::BVS(ushort* p_addr, bool* p_conditional, byte* p_bytes)
{
	BRANCH(V, p_addr, p_conditional, p_bytes);
}

void Emu6502::BNE(ushort* p_addr, bool* p_conditional, byte* p_bytes)
{
	BRANCH(!Z, p_addr, p_conditional, p_bytes);
}

void Emu6502::BEQ(ushort* p_addr, bool* p_conditional, byte* p_bytes)
{
	BRANCH(Z, p_addr, p_conditional, p_bytes);
}

void Emu6502::JSR(ushort* p_addr, byte* p_bytes)
{
	*p_bytes = 3; // for next calculation
	ushort addr2 = (ushort)(*p_addr + *p_bytes - 1);
	ushort addr3 = GetMemoryWord(*p_addr + 1);
	Push(HI(addr2));
	Push(LO(addr2));
	*p_addr = addr3;
	*p_bytes = 0; // addr already changed
}

void Emu6502::RTS(ushort* p_addr, byte* p_bytes)
{
	byte lo = Pop();
	byte hi = Pop();
	*p_addr = (ushort)(((hi << 8) | lo) + 1);
	*p_bytes = 0; // addr already changed
}

void Emu6502::RTI(ushort* p_addr, byte* p_bytes)
{
	PLP();
	byte lo = Pop();
	byte hi = Pop();
	*p_bytes = 0; // make sure caller does not increase addr by one
	*p_addr = (ushort)((hi << 8) | lo);
}

void Emu6502::BRK(byte* p_bytes)
{
	++PC;
	++PC;
	Push(HI(PC));
	Push(LO(PC));
	PHP();
	I = true;
	PC = GetMemoryWord(0xFFFE); // JMP(IRQ)
	*p_bytes = 0;
}

void Emu6502::JMP(ushort* p_addr, byte* p_bytes)
{
	*p_bytes = 0; // caller should not advance address
	ushort addr2 = GetMemoryWord(*p_addr + 1);
	*p_addr = addr2;
}

void Emu6502::JMPIND(ushort* p_addr, byte* p_bytes)
{
	*p_bytes = 0; // caller should not advance address
	ushort addr2 = GetMemoryWord(*p_addr + 1);
	ushort addr3;
	if ((addr2 & 0xFF) == 0xFF) // JMP($XXFF) won't go over page boundary
		addr3 = (ushort)(GetMemory(addr2) | (GetMemory((ushort)(addr2 - 0xFF)) << 8)); // 6502 "bug" - will use XXFF and XX00 as source of address
	else
		addr3 = GetMemoryWord(addr2);
	*p_addr = addr3;
}

// "A:FF X:FF Y:FF S:FF P:XX-XXXXX"
void Emu6502::GetDisplayState(char* state, int state_size)
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

byte Emu6502::GetIndX(ushort addr, byte* p_bytes)
{
	*p_bytes = 2;
	byte zp = (byte)(GetMemory((ushort)(addr + 1)) + X); // zero page address is byte sized
	ushort addr2 = (GetMemory(zp) | (GetMemory((byte)(zp + 1)) << 8)); // zero page address must be truncated to byte, even if wraps
	return GetMemory(addr2);
}

void Emu6502::SetIndX(byte value, ushort addr, byte* p_bytes)
{
	*p_bytes = 2;
	byte zp = (byte)(GetMemory((ushort)(addr + 1)) + X); // zero page address is byte sized
	ushort addr3 = GetMemory(zp) | (GetMemory((byte)(zp + 1)) << 8); // zero page address must be truncated to byte, even if wraps
	SetMemory(addr3, value);
}

byte Emu6502::GetIndY(ushort addr, byte* p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = (ushort)(GetMemory((ushort)(addr + 1)));
	ushort addr3 = (ushort)(GetMemoryWord(addr2) + Y);
	return GetMemory(addr3);
}

void Emu6502::SetIndY(byte value, ushort addr, byte* p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = (ushort)(GetMemory((ushort)(addr + 1)));
	ushort addr3 = (ushort)(GetMemoryWord(addr2) + Y);
	SetMemory(addr3, value);
}

byte Emu6502::GetZP(ushort addr, byte* p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = GetMemory((ushort)(addr + 1));
	return GetMemory(addr2);
}

void Emu6502::SetZP(byte value, ushort addr, byte* p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = GetMemory((ushort)(addr + 1));
	SetMemory(addr2, value);
}

byte Emu6502::GetZPX(ushort addr, byte* p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = GetMemory((ushort)(addr + 1));
	return GetMemory((byte)(addr2 + X));
}

void Emu6502::SetZPX(byte value, ushort addr, byte* p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = GetMemory((ushort)(addr + 1));
	SetMemory((byte)(addr2 + X), value);
}

byte Emu6502::GetZPY(ushort addr, byte* p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = GetMemory((ushort)(addr + 1));
	return GetMemory((byte)(addr2 + Y));
}

void Emu6502::SetZPY(byte value, ushort addr, byte* p_bytes)
{
	*p_bytes = 2;
	ushort addr2 = GetMemory((ushort)(addr + 1));
	SetMemory((byte)(addr2 + Y), value);
}

byte Emu6502::GetABS(ushort addr, byte* p_bytes)
{
	*p_bytes = 3;
	ushort addr2 = GetMemoryWord(addr + 1);
	return GetMemory(addr2);
}

void Emu6502::SetABS(byte value, ushort addr, byte* p_bytes)
{
	*p_bytes = 3;
	ushort addr2 = GetMemoryWord(addr + 1);
	SetMemory(addr2, value);
}

byte Emu6502::GetABSX(ushort addr, byte* p_bytes)
{
	*p_bytes = 3;
	ushort addr2 = GetMemoryWord(addr + 1);
	return GetMemory((ushort)(addr2 + X));
}

void Emu6502::SetABSX(byte value, ushort addr, byte* p_bytes)
{
	*p_bytes = 3;
	ushort addr2 = (ushort)(GetMemoryWord(addr + 1) + X);
	SetMemory(addr2, value);
}

byte Emu6502::GetABSY(ushort addr, byte* p_bytes)
{
	*p_bytes = 3;
	ushort addr2 = GetMemoryWord(addr + 1);
	return GetMemory((ushort)(addr2 + Y));
}

void Emu6502::SetABSY(byte value, ushort addr, byte* p_bytes)
{
	*p_bytes = 3;
	ushort addr2 = (ushort)(GetMemoryWord(addr + 1) + Y);
	SetMemory(addr2, value);
}

byte Emu6502::GetIM(ushort addr, byte* p_bytes)
{
	*p_bytes = 2;
	return GetMemory((ushort)(addr + 1));
}

void Emu6502::Execute(ushort addr)
{
	bool conditional;
	byte bytes;

	unsigned long interrupt_time = 1000000 / 60;
	unsigned long timer_then = micros();

	PC = addr;

	while (true)
	{
		while (true)
		{
			if (quit)
				return;
			bytes = 1;
			bool breakpoint = false;
			timer_now = micros();
			if (sixty_hz_irq && !I && (timer_now - timer_then) >= interrupt_time) // IRQ
			{
				timer_then = timer_now; // reset timer TODO: more accurate incremental time
				Push(HI(PC));
				Push(LO(PC));
				int flags = (N ? 0x80 : 0)
					| (V ? 0x40 : 0)
					| 0x20 // reserved, always set when pushed
					| (D ? 0x08 : 0)
					| (I ? 0x04 : 0)
					| (Z ? 0x02 : 0)
					| (C ? 0x01 : 0);
				Push(flags);
				I = true;
				PC = GetMemoryWord(0xFFFE); // JMP(IRQ)
			}
			else if (trace || breakpoint || step)
			{
				ushort addr2;
				char line[27];
				char dis[13];
				DisassembleLong(PC, &conditional, &bytes, &addr2, dis, sizeof(dis), line, sizeof(line));
				char state[33];
				GetDisplayState(state, sizeof(state));
				char full_line[80];
#ifdef _WINDOWS
				snprintf(full_line, sizeof(full_line), "%-30s%s\n", line, state);
				OutputDebugStringA(full_line);
#else				
				snprintf(full_line, sizeof(full_line), "%-30s%s\n", line, state);
				SerialDef.print(full_line);
#endif				
				if (step)
					step = step; // user can put debug breakpoint here to allow stepping
				if (breakpoint)
					breakpoint = breakpoint; // user can put debug breakpoint here to allow break
			}
			if (!ExecutePatch()) // allow execute to be overriden at a specific address
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
		case 0x24: BITOP(GetZP(PC, &bytes)); break;
		case 0x25: AND(GetZP(PC, &bytes)); break;
		case 0x26: SetZP(ROL(GetZP(PC, &bytes)), PC, &bytes); break;
		case 0x28: PLP(); break;
		case 0x29: AND(GetIM(PC, &bytes)); break;
		case 0x2A: SetA(ROL(A)); break;
		case 0x2C: BITOP(GetABS(PC, &bytes)); break;
		case 0x2D: AND(GetABS(PC, &bytes)); break;
		case 0x2E: SetABS(ROL(GetABS(PC, &bytes)), PC, &bytes); break;

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
		case 0x4E: SetABS(LSR(GetABS(PC, &bytes)), PC, &bytes); break;

		case 0x50: BVC(&PC, &conditional, &bytes); break;
		case 0x51: EOR(GetIndY(PC, &bytes)); break;
		case 0x55: EOR(GetZPX(PC, &bytes)); break;
		case 0x56: SetZPX(LSR(GetZPX(PC, &bytes)), PC, &bytes); break;
		case 0x58: CLI(); break;
		case 0x59: EOR(GetABSY(PC, &bytes)); break;
		case 0x5D: EOR(GetABSX(PC, &bytes)); break;
		case 0x5E: SetABSX(LSR(GetABSX(PC, &bytes)), PC, &bytes); break;

		case 0x60: RTS(&PC, &bytes); break;
		case 0x61: ADDC(GetIndX(PC, &bytes)); break;
		case 0x65: ADDC(GetZP(PC, &bytes)); break;
		case 0x66: SetZP(ROR(GetZP(PC, &bytes)), PC, &bytes); break;
		case 0x68: PLA(); break;
		case 0x69: ADDC(GetIM(PC, &bytes)); break;
		case 0x6A: SetA(ROR(A)); break;
		case 0x6C: JMPIND(&PC, &bytes); break;
		case 0x6D: ADDC(GetABS(PC, &bytes)); break;
		case 0x6E: SetABS(ROR(GetABS(PC, &bytes)), PC, &bytes); break;

		case 0x70: BVS(&PC, &conditional, &bytes); break;
		case 0x71: ADDC(GetIndY(PC, &bytes)); break;
		case 0x75: ADDC(GetZPX(PC, &bytes)); break;
		case 0x76: SetZPX(ROR(GetZPX(PC, &bytes)), PC, &bytes); break;
		case 0x78: SEI(); break;
		case 0x79: ADDC(GetABSY(PC, &bytes)); break;
		case 0x7D: ADDC(GetABSX(PC, &bytes)); break;
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
		case 0xC6: SetZP(DECR(GetZP(PC, &bytes)), PC, &bytes); break;
		case 0xC8: INY(); break;
		case 0xC9: CMP(GetIM(PC, &bytes)); break;
		case 0xCA: DEX(); break;
		case 0xCC: CPY(GetABS(PC, &bytes)); break;
		case 0xCD: CMP(GetABS(PC, &bytes)); break;
		case 0xCE: SetABS(DECR(GetABS(PC, &bytes)), PC, &bytes); break;

		case 0xD0: BNE(&PC, &conditional, &bytes); break;
		case 0xD1: CMP(GetIndY(PC, &bytes)); break;
		case 0xD5: CMP(GetZPX(PC, &bytes)); break;
		case 0xD6: SetZPX(DECR(GetZPX(PC, &bytes)), PC, &bytes); break;
		case 0xD8: CLD(); break;
		case 0xD9: CMP(GetABSY(PC, &bytes)); break;
		case 0xDD: CMP(GetABSX(PC, &bytes)); break;
		case 0xDE: SetABSX(DECR(GetABSX(PC, &bytes)), PC, &bytes); break;

		case 0xE0: CPX(GetIM(PC, &bytes)); break;
		case 0xE1: SBC(GetIndX(PC, &bytes)); break;
		case 0xE4: CPX(GetZP(PC, &bytes)); break;
		case 0xE5: SBC(GetZP(PC, &bytes)); break;
		case 0xE6: SetZP(INC(GetZP(PC, &bytes)), PC, &bytes); break;
		case 0xE8: INX(); break;
		case 0xE9: SBC(GetIM(PC, &bytes)); break;
		case 0xEA: NO_OP(); break;
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
			// char buffer[40];
					// snprintf(buffer, sizeof(buffer), "Invalid opcode %02X at %04X", GetMemory(PC), PC);
			//Serial.println(buffer);
			exit(1);
		}
		}

		PC += bytes;
	}
}

// Examples:
// FFFF FF FF FF JMP ($FFFF)
// FFFF FF FF FF LDA $FFFF,X
void Emu6502::DisassembleLong(ushort addr, bool* p_conditional, byte* p_bytes, ushort* p_addr2, char* dis, int dis_size, char* line, int line_size)
{
	DisassembleShort(addr, p_conditional, p_bytes, p_addr2, dis, dis_size);
	snprintf(line, line_size, "%04X ", addr);
	for (int i = 0; i < 3; ++i)
	{
		if (i < *p_bytes)
			snprintf(line + strlen(line), line_size - strlen(line), "%02X ", GetMemory((ushort)(addr + i)));
		else
			strcat_s(line, line_size, "   ");
	}
	strcat_s(line, line_size, dis);
}

void Emu6502::Ind(char* dis, int dis_size, const char* opcode, ushort addr, ushort* p_addr2, byte* p_bytes)
{
	*p_bytes = 3;
	ushort addr1 = (ushort)(GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
	*p_addr2 = (ushort)(GetMemory(addr1) | (GetMemory((ushort)(addr1 + 1)) << 8));
	snprintf(dis, dis_size, "%s ($%0X4)", opcode, addr1);
}

void Emu6502::IndX(char* dis, int dis_size, const char* opcode, ushort addr, byte* p_bytes)
{
	*p_bytes = 2;
	snprintf(dis, dis_size, "%s ($%02X,X)", opcode, GetMemory((ushort)(addr + 1)));
}

void Emu6502::IndY(char* dis, int dis_size, const char* opcode, ushort addr, byte* p_bytes)
{
	*p_bytes = 2;
	snprintf(dis, dis_size, "%s ($%02X),Y", opcode, GetMemory((ushort)(addr + 1)));
}

void Emu6502::ZP(char* dis, int dis_size, const char* opcode, ushort addr, byte* p_bytes)
{
	*p_bytes = 2;
	snprintf(dis, dis_size, "%s $%02X", opcode, GetMemory((ushort)(addr + 1)));
}

void Emu6502::ZPX(char* dis, int dis_size, const char* opcode, ushort addr, byte* p_bytes)
{
	*p_bytes = 2;
	snprintf(dis, dis_size, "%s $%02X,X", opcode, GetMemory((ushort)(addr + 1)));
}

void Emu6502::ZPY(char* dis, int dis_size, const char* opcode, ushort addr, byte* p_bytes)
{
	*p_bytes = 2;
	snprintf(dis, dis_size, "%s $%02X,Y", opcode, GetMemory((ushort)(addr + 1)));
}

void Emu6502::ABS(char* dis, int dis_size, const char* opcode, ushort addr, byte* p_bytes)
{
	*p_bytes = 3;
	snprintf(dis, dis_size, "%s $%04X", opcode, GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
}

void Emu6502::ABSAddr(char* dis, int dis_size, const char* opcode, ushort addr, ushort* p_addr2, byte* p_bytes)
{
	*p_bytes = 3;
	*p_addr2 = (ushort)(GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
	snprintf(dis, dis_size, "%s $%04X", opcode, *p_addr2);
}

void Emu6502::ABSX(char* dis, int dis_size, const char* opcode, ushort addr, byte* p_bytes)
{
	*p_bytes = 3;
	snprintf(dis, dis_size, "%s $%04X,X", opcode, GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
}

void Emu6502::ABSY(char* dis, int dis_size, const char* opcode, ushort addr, byte* p_bytes)
{
	*p_bytes = 3;
	snprintf(dis, dis_size, "%s $%04X,Y", opcode, GetMemory((ushort)(addr + 1)) | (GetMemory((ushort)(addr + 2)) << 8));
}

void Emu6502::IM(char* dis, int dis_size, const char* opcode, ushort addr, byte* p_bytes)
{
	*p_bytes = 2;
	snprintf(dis, dis_size, "%s #$%02X", opcode, GetMemory((ushort)(addr + 1)));
}

void Emu6502::BRX(char* dis, int dis_size, const char* opcode, ushort addr, bool* p_conditional, ushort* p_addr2, byte* p_bytes)
{
	*p_bytes = 2;
	*p_conditional = true;
	sbyte offset = (sbyte)GetMemory((ushort)(addr + 1));
	*p_addr2 = (ushort)(addr + 2 + offset);
	snprintf(dis, dis_size, "%s $%04X", opcode, *p_addr2);
}

// JMP ($FFFF)
// LDA $FFFF,X
void Emu6502::DisassembleShort(ushort addr, bool* p_conditional, byte* p_bytes, ushort* p_addr2, char* dis, int dis_size)
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
