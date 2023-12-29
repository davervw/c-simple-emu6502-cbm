// VDC8563 ////////////////////////////////////////////////////////////

#include "config.h"
#include "vdc.h"

const int vdc_ram_size = 64 * 1024;
const int registers_size = 38;

VDC8563::VDC8563()
{
    registers = new byte[registers_size]
    {
        126, 80, 102, 73, 32, 224, 25, 29,
        252, 231, 160, 231, 0, 0, 0, 0,
        0, 0, 15, 228, 8, 0, 120, 232,
        32, 71, 240, 0, 63, 21, 79, 0,
        0, 0, 125, 100, 245, 63
    };

    vdc_ram = new byte[vdc_ram_size];
    memset(vdc_ram, 0, vdc_ram_size);
}

VDC8563::~VDC8563()
{
    delete[] registers;
    delete[] vdc_ram;
}

byte VDC8563::GetAddressRegister()
{
    if (ready)
    {
        return 128;
    }
    else
    {
        ready = true; // simulate delay in processing
        return 0;
    }
}

void VDC8563::SetAddressRegister(byte value)
{
    register_addr = value & 0x3F;
    if (register_addr < registers_size)
        data = registers[register_addr];
    else
        data = 0xFF;
    ready = false; // simulate delay in processing
}

byte VDC8563::GetDataRegister()
{
    if (ready)
    {
        if (register_addr == 31)
        {
            ushort dest = (ushort)((registers[18] << 8) + registers[19]);
            data = vdc_ram[dest++];
            registers[18] = (byte)(dest >> 8);
            registers[19] = (byte)dest;
        }

        return data;
    }
    else
    {
        ready = true;
        return 0xFF;
    }
}

void VDC8563::SetDataRegister(byte value)
{
    ready = false; // simulate delay in processing

    if (register_addr < registers_size)
    {
        registers[register_addr] = value;

        if (register_addr == 31)
        {
            ushort dest = (ushort)((registers[18] << 8) + registers[19]);
            vdc_ram[dest++] = value;
            registers[18] = (byte)(dest >> 8);
            registers[19] = (byte)dest;
        }
        else if (register_addr == 30)
        {
            int count = (value == 0) ? 256 : value;
            ushort dest = (ushort)((registers[18] << 8) + registers[19]);
            if ((registers[24] & 0x80) == 0)
            {
                for (int i = 0; i < count; ++i)
                    vdc_ram[dest++] = registers[31];
            }
            else
            {
                ushort src = (ushort)((registers[32] << 8) + registers[33]);
                for (int i = 0; i < count; ++i)
                    vdc_ram[dest++] = vdc_ram[src++];
                registers[32] = (byte)(src >> 8);
                registers[33] = (byte)src;
            }
            registers[18] = (byte)(dest >> 8);
            registers[19] = (byte)dest;
        }
    }
}
