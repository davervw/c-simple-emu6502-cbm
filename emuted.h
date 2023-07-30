#pragma once

#include "emucbm.h"

class EmuTed : public EmuCBM 
{
public:
  class TedMemory : public Emu6502::Memory
  {
    public:
      TedMemory(int ram_size);
      virtual ~TedMemory();
      virtual byte read(ushort addr);
      virtual void write(ushort addr, byte value);

    private:
      int ram_size;
      byte* ram; // note if less than 64K, then addressing wraps around
      byte* basic_rom;
      byte* kernal_rom;
      byte* io;
      bool rom_enabled;
      int rom_config;

    private:
      TedMemory(const TedMemory& other); // disabled
      bool operator==(const TedMemory& other); // disabled
  };

public:
  EmuTed(int ram_size);
  virtual ~EmuTed();

private:
  int startup_state;
  int go_state;

protected:
  virtual bool ExecutePatch();

private:
  EmuTed(const EmuTed& other); // disabled
  bool operator==(const EmuTed& other) const; // disabled
};
