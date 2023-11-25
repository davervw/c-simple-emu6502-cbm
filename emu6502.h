// emu6502.h - Emu6502 - MOS6502 Emulator
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C++ Portable Version);
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

#pragma once

typedef signed char sbyte;
typedef unsigned char byte;
typedef unsigned short ushort;

class Emu6502
{
public:
	class Memory
	{
	public:
		Memory() {}
		virtual ~Memory() {}
		virtual byte read(ushort addr) = 0;
		virtual void write(ushort addr, byte value) = 0;

	private:
		Memory(const Memory& other); // disabled
		bool operator==(const Memory& other) const; // disabled
	};

protected:
    Memory* memory;

    byte A;
    byte X;
    byte Y;
    byte S;
    bool N;
    bool V;
    bool B;
    bool D;
    bool I;
    bool Z;
    bool C;
    ushort PC;

    bool step;
    bool quit;

    void Execute(ushort addr);
	virtual bool ExecutePatch() = 0;

	void SetA(int value);
	void Push(int value);
	byte Pop(void);
	void JSR(ushort* p_addr, byte* p_bytes);
	void RTS(ushort* p_addr, byte* p_bytes);

	typedef void (Emu6502::*ExecuteMethod)(byte* p_bytes);
	ExecuteMethod ExecuteOp[256];

public:
    bool trace;

    Emu6502(Memory* memory);
	virtual ~Emu6502();
    void ResetRun();

	byte LO(ushort value);
	byte HI(ushort value);
	void DisassembleLong(ushort addr, bool* p_conditional, byte* p_bytes, ushort* p_addr2, char* dis, int dis_size, char* line, int line_size);
	void DisassembleShort(ushort addr, bool* p_conditional, byte* p_bytes, ushort* p_addr2, char* dis, int dis_size);
	byte GetMemory(ushort addr);
	void SetMemory(ushort addr, byte value);

private:
	Emu6502(const Emu6502& other); // disabled
	bool operator==(const Emu6502& other) const; // disabled

private:
	void PHP();
	byte Subtract(byte reg, byte value, bool* p_overflow);
	byte SubtractWithoutOverflow(byte reg, byte value);
	void CMP(byte value);
	void CPX(byte value);
	void CPY(byte value);
	void SetReg(byte* p_reg, int value);
	void SetX(int value);
	void SetY(int value);
	void SBC(byte value);
	void ADC(byte value);
	void ORA(int value);
	void EOR(int value);
	void AND(int value);
	void BIT(byte value);
	byte ASL(int value);
	byte LSR(int value);
	byte ROL(int value);
	byte ROR(int value);
	void PLP();
	void PHA();
	void PLA();
	void CLC();
	void CLD();
	void CLI();
	void CLV();
	void SEC();
	void SED();
	void SEI();
	byte INC(byte value);
	void INX();
	void INY();
	byte DEC(byte value);
	void DEX();
	void DEY();
	void NOP();
	void TXA();
	void TAX();
	void TYA();
	void TAY();
	void TXS();
	void TSX();
	ushort GetBR(ushort addr, byte* p_bytes);
	void BR(bool branch, ushort* p_addr, byte* p_bytes);
	void BPL(ushort* p_addr, byte* p_bytes);
	void BMI(ushort* p_addr, byte* p_bytes);
	void BCC(ushort* p_addr, byte* p_bytes);
	void BCS(ushort* p_addr, byte* p_bytes);
	void BVC(ushort* p_addr, byte* p_bytes);
	void BVS(ushort* p_addr, byte* p_bytes);
	void BNE(ushort* p_addr, byte* p_bytes);
	void BEQ(ushort* p_addr, byte* p_bytes);
	void RTI(ushort* p_addr, byte* p_bytes);
	void BRK(byte* p_bytes);
	void JMP(ushort* p_addr, byte* p_bytes);
	void JMPIND(ushort* p_addr, byte* p_bytes);
	void GetDisplayState(char* state, int state_size);
	byte GetIndX(ushort addr, byte* p_bytes);
	void SetIndX(byte value, ushort addr, byte* p_bytes);
	byte GetIndY(ushort addr, byte* p_bytes);
	void SetIndY(byte value, ushort addr, byte* p_bytes);
	byte GetZP(ushort addr, byte* p_bytes);
	void SetZP(byte value, ushort addr, byte* p_bytes);
	byte GetZPX(ushort addr, byte* p_bytes);
	void SetZPX(byte value, ushort addr, byte* p_bytes);
	byte GetZPY(ushort addr, byte* p_bytes);
	void SetZPY(byte value, ushort addr, byte* p_bytes);
	byte GetABS(ushort addr, byte* p_bytes);
	void SetABS(byte value, ushort addr, byte* p_bytes);
	byte GetABSX(ushort addr, byte* p_bytes);
	void SetABSX(byte value, ushort addr, byte* p_bytes);
	byte GetABSY(ushort addr, byte* p_bytes);
	void SetABSY(byte value, ushort addr, byte* p_bytes);
	byte GetIM(ushort addr, byte* p_bytes);
	void Ind(char* dis, int dis_size, const char* opcode, ushort addr, ushort* p_addr2, byte* p_bytes);
	void IndX(char* dis, int dis_size, const char* opcode, ushort addr, byte* p_bytes);
	void IndY(char* dis, int dis_size, const char* opcode, ushort addr, byte* p_bytes);
	void ZP(char* dis, int dis_size, const char* opcode, ushort addr, byte* p_bytes);
	void ZPX(char* dis, int dis_size, const char* opcode, ushort addr, byte* p_bytes);
	void ZPY(char* dis, int dis_size, const char* opcode, ushort addr, byte* p_bytes);
	void ABS(char* dis, int dis_size, const char* opcode, ushort addr, byte* p_bytes);
	void ABSAddr(char* dis, int dis_size, const char* opcode, ushort addr, ushort* p_addr2, byte* p_bytes);
	void ABSX(char* dis, int dis_size, const char* opcode, ushort addr, byte* p_bytes);
	void ABSY(char* dis, int dis_size, const char* opcode, ushort addr, byte* p_bytes);
	void IM(char* dis, int dis_size, const char* opcode, ushort addr, byte* p_bytes);
	void BRX(char* dis, int dis_size, const char* opcode, ushort addr, bool* p_conditional, ushort* p_addr2, byte* p_bytes);

	void ExecuteOp00(byte* p_bytes);
	void ExecuteOp01(byte* p_bytes);
	void ExecuteOp02(byte* p_bytes);
	void ExecuteOp03(byte* p_bytes);
	void ExecuteOp04(byte* p_bytes);
	void ExecuteOp05(byte* p_bytes);
	void ExecuteOp06(byte* p_bytes);
	void ExecuteOp07(byte* p_bytes);
	void ExecuteOp08(byte* p_bytes);
	void ExecuteOp09(byte* p_bytes);
	void ExecuteOp0A(byte* p_bytes);
	void ExecuteOp0B(byte* p_bytes);
	void ExecuteOp0C(byte* p_bytes);
	void ExecuteOp0D(byte* p_bytes);
	void ExecuteOp0E(byte* p_bytes);
	void ExecuteOp0F(byte* p_bytes);

	void ExecuteOp10(byte* p_bytes);
	void ExecuteOp11(byte* p_bytes);
	void ExecuteOp12(byte* p_bytes);
	void ExecuteOp13(byte* p_bytes);
	void ExecuteOp14(byte* p_bytes);
	void ExecuteOp15(byte* p_bytes);
	void ExecuteOp16(byte* p_bytes);
	void ExecuteOp17(byte* p_bytes);
	void ExecuteOp18(byte* p_bytes);
	void ExecuteOp19(byte* p_bytes);
	void ExecuteOp1A(byte* p_bytes);
	void ExecuteOp1B(byte* p_bytes);
	void ExecuteOp1C(byte* p_bytes);
	void ExecuteOp1D(byte* p_bytes);
	void ExecuteOp1E(byte* p_bytes);
	void ExecuteOp1F(byte* p_bytes);

	void ExecuteOp20(byte* p_bytes);
	void ExecuteOp21(byte* p_bytes);
	void ExecuteOp22(byte* p_bytes);
	void ExecuteOp23(byte* p_bytes);
	void ExecuteOp24(byte* p_bytes);
	void ExecuteOp25(byte* p_bytes);
	void ExecuteOp26(byte* p_bytes);
	void ExecuteOp27(byte* p_bytes);
	void ExecuteOp28(byte* p_bytes);
	void ExecuteOp29(byte* p_bytes);
	void ExecuteOp2A(byte* p_bytes);
	void ExecuteOp2B(byte* p_bytes);
	void ExecuteOp2C(byte* p_bytes);
	void ExecuteOp2D(byte* p_bytes);
	void ExecuteOp2E(byte* p_bytes);
	void ExecuteOp2F(byte* p_bytes);

	void ExecuteOp30(byte* p_bytes);
	void ExecuteOp31(byte* p_bytes);
	void ExecuteOp32(byte* p_bytes);
	void ExecuteOp33(byte* p_bytes);
	void ExecuteOp34(byte* p_bytes);
	void ExecuteOp35(byte* p_bytes);
	void ExecuteOp36(byte* p_bytes);
	void ExecuteOp37(byte* p_bytes);
	void ExecuteOp38(byte* p_bytes);
	void ExecuteOp39(byte* p_bytes);
	void ExecuteOp3A(byte* p_bytes);
	void ExecuteOp3B(byte* p_bytes);
	void ExecuteOp3C(byte* p_bytes);
	void ExecuteOp3D(byte* p_bytes);
	void ExecuteOp3E(byte* p_bytes);
	void ExecuteOp3F(byte* p_bytes);

	void ExecuteOp40(byte* p_bytes);
	void ExecuteOp41(byte* p_bytes);
	void ExecuteOp42(byte* p_bytes);
	void ExecuteOp43(byte* p_bytes);
	void ExecuteOp44(byte* p_bytes);
	void ExecuteOp45(byte* p_bytes);
	void ExecuteOp46(byte* p_bytes);
	void ExecuteOp47(byte* p_bytes);
	void ExecuteOp48(byte* p_bytes);
	void ExecuteOp49(byte* p_bytes);
	void ExecuteOp4A(byte* p_bytes);
	void ExecuteOp4B(byte* p_bytes);
	void ExecuteOp4C(byte* p_bytes);
	void ExecuteOp4D(byte* p_bytes);
	void ExecuteOp4E(byte* p_bytes);
	void ExecuteOp4F(byte* p_bytes);

	void ExecuteOp50(byte* p_bytes);
	void ExecuteOp51(byte* p_bytes);
	void ExecuteOp52(byte* p_bytes);
	void ExecuteOp53(byte* p_bytes);
	void ExecuteOp54(byte* p_bytes);
	void ExecuteOp55(byte* p_bytes);
	void ExecuteOp56(byte* p_bytes);
	void ExecuteOp57(byte* p_bytes);
	void ExecuteOp58(byte* p_bytes);
	void ExecuteOp59(byte* p_bytes);
	void ExecuteOp5A(byte* p_bytes);
	void ExecuteOp5B(byte* p_bytes);
	void ExecuteOp5C(byte* p_bytes);
	void ExecuteOp5D(byte* p_bytes);
	void ExecuteOp5E(byte* p_bytes);
	void ExecuteOp5F(byte* p_bytes);

	void ExecuteOp60(byte* p_bytes);
	void ExecuteOp61(byte* p_bytes);
	void ExecuteOp62(byte* p_bytes);
	void ExecuteOp63(byte* p_bytes);
	void ExecuteOp64(byte* p_bytes);
	void ExecuteOp65(byte* p_bytes);
	void ExecuteOp66(byte* p_bytes);
	void ExecuteOp67(byte* p_bytes);
	void ExecuteOp68(byte* p_bytes);
	void ExecuteOp69(byte* p_bytes);
	void ExecuteOp6A(byte* p_bytes);
	void ExecuteOp6B(byte* p_bytes);
	void ExecuteOp6C(byte* p_bytes);
	void ExecuteOp6D(byte* p_bytes);
	void ExecuteOp6E(byte* p_bytes);
	void ExecuteOp6F(byte* p_bytes);

	void ExecuteOp70(byte* p_bytes);
	void ExecuteOp71(byte* p_bytes);
	void ExecuteOp72(byte* p_bytes);
	void ExecuteOp73(byte* p_bytes);
	void ExecuteOp74(byte* p_bytes);
	void ExecuteOp75(byte* p_bytes);
	void ExecuteOp76(byte* p_bytes);
	void ExecuteOp77(byte* p_bytes);
	void ExecuteOp78(byte* p_bytes);
	void ExecuteOp79(byte* p_bytes);
	void ExecuteOp7A(byte* p_bytes);
	void ExecuteOp7B(byte* p_bytes);
	void ExecuteOp7C(byte* p_bytes);
	void ExecuteOp7D(byte* p_bytes);
	void ExecuteOp7E(byte* p_bytes);
	void ExecuteOp7F(byte* p_bytes);

	void ExecuteOp80(byte* p_bytes);
	void ExecuteOp81(byte* p_bytes);
	void ExecuteOp82(byte* p_bytes);
	void ExecuteOp83(byte* p_bytes);
	void ExecuteOp84(byte* p_bytes);
	void ExecuteOp85(byte* p_bytes);
	void ExecuteOp86(byte* p_bytes);
	void ExecuteOp87(byte* p_bytes);
	void ExecuteOp88(byte* p_bytes);
	void ExecuteOp89(byte* p_bytes);
	void ExecuteOp8A(byte* p_bytes);
	void ExecuteOp8B(byte* p_bytes);
	void ExecuteOp8C(byte* p_bytes);
	void ExecuteOp8D(byte* p_bytes);
	void ExecuteOp8E(byte* p_bytes);
	void ExecuteOp8F(byte* p_bytes);

	void ExecuteOp90(byte* p_bytes);
	void ExecuteOp91(byte* p_bytes);
	void ExecuteOp92(byte* p_bytes);
	void ExecuteOp93(byte* p_bytes);
	void ExecuteOp94(byte* p_bytes);
	void ExecuteOp95(byte* p_bytes);
	void ExecuteOp96(byte* p_bytes);
	void ExecuteOp97(byte* p_bytes);
	void ExecuteOp98(byte* p_bytes);
	void ExecuteOp99(byte* p_bytes);
	void ExecuteOp9A(byte* p_bytes);
	void ExecuteOp9B(byte* p_bytes);
	void ExecuteOp9C(byte* p_bytes);
	void ExecuteOp9D(byte* p_bytes);
	void ExecuteOp9E(byte* p_bytes);
	void ExecuteOp9F(byte* p_bytes);

	void ExecuteOpA0(byte* p_bytes);
	void ExecuteOpA1(byte* p_bytes);
	void ExecuteOpA2(byte* p_bytes);
	void ExecuteOpA3(byte* p_bytes);
	void ExecuteOpA4(byte* p_bytes);
	void ExecuteOpA5(byte* p_bytes);
	void ExecuteOpA6(byte* p_bytes);
	void ExecuteOpA7(byte* p_bytes);
	void ExecuteOpA8(byte* p_bytes);
	void ExecuteOpA9(byte* p_bytes);
	void ExecuteOpAA(byte* p_bytes);
	void ExecuteOpAB(byte* p_bytes);
	void ExecuteOpAC(byte* p_bytes);
	void ExecuteOpAD(byte* p_bytes);
	void ExecuteOpAE(byte* p_bytes);
	void ExecuteOpAF(byte* p_bytes);

	void ExecuteOpB0(byte* p_bytes);
	void ExecuteOpB1(byte* p_bytes);
	void ExecuteOpB2(byte* p_bytes);
	void ExecuteOpB3(byte* p_bytes);
	void ExecuteOpB4(byte* p_bytes);
	void ExecuteOpB5(byte* p_bytes);
	void ExecuteOpB6(byte* p_bytes);
	void ExecuteOpB7(byte* p_bytes);
	void ExecuteOpB8(byte* p_bytes);
	void ExecuteOpB9(byte* p_bytes);
	void ExecuteOpBA(byte* p_bytes);
	void ExecuteOpBB(byte* p_bytes);
	void ExecuteOpBC(byte* p_bytes);
	void ExecuteOpBD(byte* p_bytes);
	void ExecuteOpBE(byte* p_bytes);
	void ExecuteOpBF(byte* p_bytes);

	void ExecuteOpC0(byte* p_bytes);
	void ExecuteOpC1(byte* p_bytes);
	void ExecuteOpC2(byte* p_bytes);
	void ExecuteOpC3(byte* p_bytes);
	void ExecuteOpC4(byte* p_bytes);
	void ExecuteOpC5(byte* p_bytes);
	void ExecuteOpC6(byte* p_bytes);
	void ExecuteOpC7(byte* p_bytes);
	void ExecuteOpC8(byte* p_bytes);
	void ExecuteOpC9(byte* p_bytes);
	void ExecuteOpCA(byte* p_bytes);
	void ExecuteOpCB(byte* p_bytes);
	void ExecuteOpCC(byte* p_bytes);
	void ExecuteOpCD(byte* p_bytes);
	void ExecuteOpCE(byte* p_bytes);
	void ExecuteOpCF(byte* p_bytes);

	void ExecuteOpD0(byte* p_bytes);
	void ExecuteOpD1(byte* p_bytes);
	void ExecuteOpD2(byte* p_bytes);
	void ExecuteOpD3(byte* p_bytes);
	void ExecuteOpD4(byte* p_bytes);
	void ExecuteOpD5(byte* p_bytes);
	void ExecuteOpD6(byte* p_bytes);
	void ExecuteOpD7(byte* p_bytes);
	void ExecuteOpD8(byte* p_bytes);
	void ExecuteOpD9(byte* p_bytes);
	void ExecuteOpDA(byte* p_bytes);
	void ExecuteOpDB(byte* p_bytes);
	void ExecuteOpDC(byte* p_bytes);
	void ExecuteOpDD(byte* p_bytes);
	void ExecuteOpDE(byte* p_bytes);
	void ExecuteOpDF(byte* p_bytes);

	void ExecuteOpE0(byte* p_bytes);
	void ExecuteOpE1(byte* p_bytes);
	void ExecuteOpE2(byte* p_bytes);
	void ExecuteOpE3(byte* p_bytes);
	void ExecuteOpE4(byte* p_bytes);
	void ExecuteOpE5(byte* p_bytes);
	void ExecuteOpE6(byte* p_bytes);
	void ExecuteOpE7(byte* p_bytes);
	void ExecuteOpE8(byte* p_bytes);
	void ExecuteOpE9(byte* p_bytes);
	void ExecuteOpEA(byte* p_bytes);
	void ExecuteOpEB(byte* p_bytes);
	void ExecuteOpEC(byte* p_bytes);
	void ExecuteOpED(byte* p_bytes);
	void ExecuteOpEE(byte* p_bytes);
	void ExecuteOpEF(byte* p_bytes);

	void ExecuteOpF0(byte* p_bytes);
	void ExecuteOpF1(byte* p_bytes);
	void ExecuteOpF2(byte* p_bytes);
	void ExecuteOpF3(byte* p_bytes);
	void ExecuteOpF4(byte* p_bytes);
	void ExecuteOpF5(byte* p_bytes);
	void ExecuteOpF6(byte* p_bytes);
	void ExecuteOpF7(byte* p_bytes);
	void ExecuteOpF8(byte* p_bytes);
	void ExecuteOpF9(byte* p_bytes);
	void ExecuteOpFA(byte* p_bytes);
	void ExecuteOpFB(byte* p_bytes);
	void ExecuteOpFC(byte* p_bytes);
	void ExecuteOpFD(byte* p_bytes);
	void ExecuteOpFE(byte* p_bytes);
	void ExecuteOpFF(byte* p_bytes);

	void InvalidOpcode();
};
