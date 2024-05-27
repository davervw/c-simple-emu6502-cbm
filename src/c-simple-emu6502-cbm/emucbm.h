// emucbm.h - class EmuCBM - Commodore Business Machines Emulator
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

#include "emu6502.h"
#include "emud64.h"

class EmuCBM : public Emu6502
{
public:
  EmuCBM(Memory* memory);
  virtual ~EmuCBM();
  static void File_ReadAllBytes(byte* bytes, int size, const char* filename);

protected:
  int LOAD_TRAP;
  const char* FileName;
  byte FileNum;
  byte FileDev;
  byte FileSec;
  bool FileVerify;
  ushort FileAddr;

protected:
  EmuD64* GetDisk();
  byte* OpenRead(const char* filename, int* p_ret_file_len);
  bool FileLoad(byte* p_err);
  bool FileSave(const char* filename, ushort addr1, ushort addr2);
  bool LoadStartupPrg();
  bool ExecutePatch();
  bool ExecuteRTS();
  bool ExecuteJSR(ushort addr);
};