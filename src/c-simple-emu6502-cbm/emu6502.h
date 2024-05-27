// emu6502.h - Emu6502 - MOS6502 Emulator
//
////////////////////////////////////////////////////////////////////////////////
//
// c-simple-emu-cbm (C Portable Version);
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

#pragma once

//#define TEST6502 // set for 6502 tests instead of Commodore emulation, see emutest.cpp

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

public:
  Emu6502(Memory* memory);
  virtual ~Emu6502();
  void ResetRun();

protected:
  Memory* memory;
  byte GetMemory(ushort addr)
  {
    return memory->read(addr);
  }
  ushort GetMemoryWord(ushort addr)
  {
    return memory->read(addr) | (memory->read(addr+1) << 8);
  }
  void SetMemory(ushort addr, byte value)
  {
    memory->write(addr, value);
  }
  virtual bool ExecutePatch() = 0;
  void Push(int value);
  byte Pop(void);
  // void DisassembleLong(ushort addr, bool *p_conditional, byte *p_bytes, ushort *p_addr2, char *dis, int dis_size, char *line, int line_size);
  // void DisassembleShort(ushort addr, bool *p_conditional, byte *p_bytes, ushort *p_addr2, char *dis, int dis_size);
  byte LO(ushort value);
  byte HI(ushort value);
  void PHP();
  void SetA(int value);
  void RTS(ushort* p_addr, byte* p_bytes);

protected:
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

  bool trace;
  bool step;
  bool quit;

  bool sixty_hz_irq; // TODO: add external timer source object, shouldn't be part of 6502
  unsigned long timer_now; // TODO: move to internal timing object

private:
  byte Subtract(byte reg, byte value, bool *p_overflow);
  byte SubtractWithoutOverflow(byte reg, byte value);
  void CMP(byte value);
  void CPX(byte value);
  void CPY(byte value);
  void SetReg(byte *p_reg, int value);
  void SetX(int value);
  void SetY(int value);
  void SBC(byte value);
  void ADDC(byte value);
  void ORA(int value);
  void EOR(int value);
  void AND(int value);
  void BITOP(byte value);
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
  byte DECR(byte value);
  void DEX();
  void DEY();
  void NO_OP();
  void TXA();
  void TAX();
  void TYA();
  void TAY();
  void TXS();
  void TSX();
  ushort GetBR(ushort addr, bool *p_conditional, byte *p_bytes);
  void BRANCH(bool branch, ushort *p_addr, bool *p_conditional, byte *p_bytes);
  void BPL(ushort *p_addr, bool *p_conditional, byte *p_bytes);
  void BMI(ushort *p_addr, bool *p_conditional, byte *p_bytes);
  void BCC(ushort *p_addr, bool *p_conditional, byte *p_bytes);
  void BCS(ushort *p_addr, bool *p_conditional, byte *p_bytes);
  void BVC(ushort *p_addr, bool *p_conditional, byte *p_bytes);
  void BVS(ushort *p_addr, bool *p_conditional, byte *p_bytes);
  void BNE(ushort *p_addr, bool *p_conditional, byte *p_bytes);
  void BEQ(ushort *p_addr, bool *p_conditional, byte *p_bytes);
  void JSR(ushort *p_addr, byte *p_bytes);
  void RTI(ushort *p_addr, byte *p_bytes);
  void BRK(byte *p_bytes);
  void JMP(ushort *p_addr, byte *p_bytes);
  void JMPIND(ushort *p_addr, byte *p_bytes);
  byte GetIndX(ushort addr, byte *p_bytes);
  void SetIndX(byte value, ushort addr, byte *p_bytes);
  byte GetIndY(ushort addr, byte *p_bytes);
  void SetIndY(byte value, ushort addr, byte *p_bytes);
  byte GetZP(ushort addr, byte *p_bytes);
  void SetZP(byte value, ushort addr, byte *p_bytes);
  byte GetZPX(ushort addr, byte *p_bytes);
  void SetZPX(byte value, ushort addr, byte *p_bytes);
  byte GetZPY(ushort addr, byte *p_bytes);
  void SetZPY(byte value, ushort addr, byte *p_bytes);
  byte GetABS(ushort addr, byte *p_bytes);
  void SetABS(byte value, ushort addr, byte *p_bytes);
  byte GetABSX(ushort addr, byte *p_bytes);
  void SetABSX(byte value, ushort addr, byte *p_bytes);
  byte GetABSY(ushort addr, byte *p_bytes);
  void SetABSY(byte value, ushort addr, byte *p_bytes);
  byte GetIM(ushort addr, byte *p_bytes);
  void Execute(ushort addr);

  void GetDisplayState(char *state, int state_size);
  void DisassembleShort(ushort addr, bool *p_conditional, byte *p_bytes, ushort *p_addr2, char *dis, int dis_size);
  void DisassembleLong(ushort addr, bool *p_conditional, byte *p_bytes, ushort *p_addr2, char *dis, int dis_size, char *line, int line_size);

  void Ind(char *dis, int dis_size, const char* opcode, ushort addr, ushort *p_addr2, byte *p_bytes);
  void IndX(char *dis, int dis_size, const char* opcode, ushort addr, byte *p_bytes);
  void IndY(char *dis, int dis_size, const char* opcode, ushort addr, byte *p_bytes);
  void ZP(char *dis, int dis_size, const char* opcode, ushort addr, byte *p_bytes);
  void ZPX(char *dis, int dis_size, const char* opcode, ushort addr, byte *p_bytes);
  void ZPY(char *dis, int dis_size, const char* opcode, ushort addr, byte *p_bytes);
  void ABS(char *dis, int dis_size, const char* opcode, ushort addr, byte *p_bytes);
  void ABSAddr(char *dis, int dis_size, const char* opcode, ushort addr, ushort *p_addr2, byte *p_bytes);
  void ABSX(char *dis, int dis_size, const char* opcode, ushort addr, byte *p_bytes);
  void ABSY(char *dis, int dis_size, const char* opcode, ushort addr, byte *p_bytes);
  void IM(char *dis, int dis_size, const char* opcode, ushort addr, byte *p_bytes);
  void BRX(char *dis, int dis_size, const char* opcode, ushort addr, bool *p_conditional, ushort *p_addr2, byte *p_bytes);

private:
  Emu6502(const Emu6502& other); // disabled
  bool operator==(const Emu6502& other) const; // disabled
};
