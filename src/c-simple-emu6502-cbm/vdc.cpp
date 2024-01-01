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

void VDC8563::Activate()
{
  if (!active)
  {
    active = true;
    int bg = VDCColorToLCDColor(registers[26]);
#ifdef ARDUINO_SUNTON_8048S070
    gfx->fillRect(0, 0, 800, 480, bg);
#endif    
#ifdef ILI9488
    lcd.fillRect(0, 0, 320, 480, bg);
#endif
    RedrawScreen();
  }
}

void VDC8563::Deactivate()
{
  active = false;
}

int VDC8563::VDCColorToLCDColor(byte value)
{
  switch (value & 15)
  {
    case 0: return 0x0000; // BLACK
#ifdef ARDUINO_SUNTON_8048S070
    case 1: return DARKGREY;
    case 2: return BLUE;
    case 3: return 0x841F; // LIGHTBLUE
    case 4: return 0x0400; // DARKGREEN
    case 5: return 0xBFF7; // LIGHTGREEN
    case 6: return DARKCYAN; // MED GRAY
    case 7: return CYAN;
    case 8: return RED;
    case 9: return 0xFC10; // PINK OR LT RED
    case 10: return 0x8118; // DARKMAGENTA OR PURPLE
    case 11: return 0xF81F; // LIGHT MAGENTA
    case 12: return 0x8283; // BROWN;
    case 13: return YELLOW;
    case 14: return LIGHTGREY;
    case 15: return WHITE;
#endif
#ifdef ILI9488    
    case 1: return ILI9488_DARKGREY;
    case 2: return ILI9488_BLUE;
    case 3: return 0x841F; // LIGHTBLUE
    case 4: return 0x0400; // DARKGREEN
    case 5: return 0xBFF7; // LIGHTGREEN
    case 6: return ILI9488_DARKCYAN; // MED GRAY
    case 7: return ILI9488_CYAN;
    case 8: return ILI9488_RED;
    case 9: return 0xFC10; // PINK OR LT RED
    case 10: return 0x8118; // DARKMAGENTA OR PURPLE
    case 11: return 0xF81F; // LIGHT MAGENTA
    case 12: return 0x8283; // BROWN;
    case 13: return ILI9488_YELLOW;
    case 14: return ILI9488_LIGHTGREY;
    case 15: return ILI9488_WHITE;
#endif  
    default: return 0xFFFF; // WHITE
  }
}

#ifdef ILI9488
int static average_color(int color0, int color2)
{
  if (color0 == color2)
    return color0;  
  unsigned char r0, g0, b0, r1, g1, b1, r2, g2, b2;
  lcd.color565toRGB(color0, r0, g0, b0);
  lcd.color565toRGB(color2, r2, g2, b2);
  r1 = (r0+r2)/2;
  g1 = (g0+g2)/2;
  b1 = (b0+b2)/2;
  int color1 = CL(r1, g1, b1);
  return color1;
}
#endif

void VDC8563::DrawChar(byte c, int col, int row, int fg, int bg, byte attrib)
{
  if (!active)
    return;

#ifdef ARDUINO_SUNTON_8048S070      
  int x0 = 80 + col * 8;
  int y0 = 40 + row * 16;
#endif
#ifdef ILI9488
  int x0 = 10 + row * 12;
  int y0 = col * 6;
  #define SCALEY(n) ((n*3+1)/2)
  int colors[8];
  bool fill = true;
#endif
  byte* src = &vdc_ram[0x2000] + 16 * c + 4096 * (attrib >> 7);
  bool inverse = ((registers[24] & 0x40) != 0);
  byte cursorMode = (registers[10] >> 5) & 3;
  bool isCursor = ( row*80+col == (registers[14] << 8 | registers[15]) && cursorMode != 1 );
  for (int row_i=0; row_i<8; ++row_i)
  {
    int mask = 128;
    bool underlined = ( ( (attrib & 0x20) != 0 ) && ( row_i == (registers[29] & 0x1F) ) );
    for (int col_i=0; col_i<8; ++col_i)
    {
      int color = (((src[row_i] & mask) != 0 || underlined) ^ inverse ^ isCursor) ? fg : bg;
#ifdef ARDUINO_SUNTON_8048S070      
      gfx->drawPixel(x0+col_i, y0+row_i*2, color);
      gfx->drawPixel(x0+col_i, y0+row_i*2+1, color);
#endif      
#ifdef ILI9488
      if (col_i >= 2 && col_i <= 5)
      {
        if (fill && (row_i & 1) == 1)
          lcd.drawPixel(x0+SCALEY(row_i)-1, 479-(y0+col_i-1), average_color(color, colors[col_i]));
        lcd.drawPixel(x0+SCALEY(row_i), 479-(y0+col_i-1), color);
      }
      else if (col_i == 1 || col_i == 7)
      {
        int adj = (col_i == 7) ? 1 : 0;
        int mid = average_color(color, colors[col_i-1]);
        if (fill && (row_i & 1) == 1)
        {
          int midabove = average_color(colors[col_i], (((src[row_i-1] & (mask << 1)) != 0 || underlined) ^ inverse ^ isCursor) ? fg : bg);
          lcd.drawPixel(x0+SCALEY(row_i)-1, 479-(y0+col_i-1-adj), average_color(mid, midabove));
        }
        lcd.drawPixel(x0+SCALEY(row_i), 479-(y0+col_i-1-adj), mid);
      }
      colors[col_i] = color;
#endif
      mask >>= 1;
    }
  }
}

void VDC8563::DrawChar(int offset)
{
  int col = offset % 80;
  int row = offset / 80;
  bool attributesEnabled = ((registers[25] & 0x40) != 0);
  byte attrib = vdc_ram[2048+offset];
  int fg = attributesEnabled ? VDCColorToLCDColor(attrib) : VDCColorToLCDColor(registers[26] >> 4);
  int bg = VDCColorToLCDColor(registers[26]);
  DrawChar(vdc_ram[offset], col, row, fg, bg, vdc_ram[offset+0x800]);
}

void VDC8563::RedrawScreen()
{
  if (!active)
    return;

#ifdef M5STACK   
  M5.Lcd.startWrite();
#endif  
  int bg = VDCColorToLCDColor(registers[26]);
  int offset = 0;
  bool attributesEnabled = ((registers[25] & 0x40) != 0);
  for (int row = 0; row < 25; ++row)
  {
    for (int col = 0; col < 80; ++col)
    {
      byte attrib = vdc_ram[2048+offset];
      int fg = attributesEnabled ? VDCColorToLCDColor(attrib) : VDCColorToLCDColor(registers[26] >> 4);
      DrawChar(vdc_ram[offset], col, row, fg, bg, attrib);
      ++offset;
    }
  }
#ifdef M5STACK  
  M5.Lcd.endWrite();
#endif  
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
    ready = true;
    if (register_addr >= registers_size)
      return;
    if (register_addr == 31)
    {
        registers[register_addr] = value;
        ushort dest = (ushort)((registers[18] << 8) + registers[19]);
        if (vdc_ram[dest] != value) // optimize drawing
        {
          vdc_ram[dest] = value;
          if (active)
          {
            if (dest >= 0 && dest < 2000)
              DrawChar(dest);
            else if (dest >= 2048 && dest < 4048)
              DrawChar(dest & 0x7FF);
          }
        }
        ++dest;
        registers[18] = (byte)(dest >> 8);
        registers[19] = (byte)dest;
    }
    else if (register_addr == 30)
    {
        registers[register_addr] = value;
        int count = (value == 0) ? 256 : value;
        ushort dest = (ushort)((registers[18] << 8) + registers[19]);
        bool fill = ((registers[24] & 0x80) == 0);
        bool copy = !fill;
        if (fill)
        {
            for (int i = 0; i < count; ++i)
            {
                vdc_ram[dest] = registers[31];
                if (active)
                {
                  if (dest >= 0 && dest < 2000)
                    DrawChar(dest);
                  else if (dest >= 2048 && dest < 4048)
                    DrawChar(dest & 0x7FF);
                }
                ++dest;
            }
        }
        else if (copy)
        {
            ushort src = (ushort)((registers[32] << 8) + registers[33]);
            for (int i = 0; i < count; ++i)
            {
                if (vdc_ram[dest] != vdc_ram[src]) // optimize drawing
                {
                  vdc_ram[dest] = vdc_ram[src];
                  if (active)
                  {
                    if (dest >= 0 && dest < 2000)
                      DrawChar(dest);
                    else if (dest >= 2048 && dest < 4048)
                      DrawChar(dest & 0x7FF);
                  }
                }
                ++dest;
                ++src;
            }
            registers[32] = (byte)(src >> 8);
            registers[33] = (byte)src;
        }
        registers[18] = (byte)(dest >> 8);
        registers[19] = (byte)dest;
    }
    else if (register_addr == 24)
    {
      bool reverseScreenChange = ((registers[register_addr] & 0x40) != (value & 0x40));
      registers[register_addr] = value;
      if (reverseScreenChange)
        RedrawScreen();
    }
    else if (register_addr == 25)
    {
      bool attributesEnableChange = ((registers[register_addr] & 0x40) != (value & 0x40));
      registers[register_addr] = value;
      if (attributesEnableChange)
        RedrawScreen();
    }
    else if (register_addr == 26)
    {
      bool attributesEnabled = ((registers[25] & 0x40) != 0);
      bool colorScreenChange = (attributesEnabled && (registers[register_addr] & 0xF) != (value & 0xF))
        || (!attributesEnabled && registers[register_addr] != value);
      registers[register_addr] = value;
      if (colorScreenChange)
      {
        int bg = VDCColorToLCDColor(registers[26]);
#ifdef ARDUINO_SUNTON_8048S070
        gfx->fillRect(0, 0, 800, 480, bg);
#endif    
#ifdef ILI9488
        lcd.fillRect(0, 0, 320, 480, bg);
#endif
        RedrawScreen();
      }
    }
    else if (register_addr == 10)
    {
      bool cursorChanged = ((registers[register_addr] & 0x7F) != (value & 0x7F));
      registers[register_addr] = value;
      if (cursorChanged)
        DrawChar((registers[14] << 8) | registers[15]);
    }
    else if (register_addr == 15)
    {
      registers[register_addr] = value;
      DrawChar((registers[14] << 8) | registers[15]);
    }
    else
      registers[register_addr] = value;
}
