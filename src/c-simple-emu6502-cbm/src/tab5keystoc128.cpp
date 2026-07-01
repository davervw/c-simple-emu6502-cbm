#include "tab5keystoc128.h"

namespace
{
    // Tab5 Keyboard Layout (label)
    // esc  1   2 3 4 5 6 7 8  9  0  -  +  del
    // `~   !?  @ # $ % ^ & */ (< )> [{ ]} \|
    // tab  q   w e r t y u i  o  p  ;: '" bs
    // sym  Aa  a s d f g h j  k  l  up _= return
    // ctrl alt z x c v b n m  ., lt dn rt space

    // [C128 unshifted]
    // stop 1   2 3 4 5 6 7 8  9  0  -  +  bs
    // `    !   @ # $ % ↑ & *  (  )  [  ]  £
    // CBM  q   w e r t y u i  o  p  ;  '  bs
    // SYM  LSH a s d f g h j  k  l  up ←  return  LSH = LEFT SHIFT
    // CTRL ALT z x c v b n m  .  lt dn rt space

    // [C128 SYM+]
    // stopF1..F2F3F4F5F6F7F8          home restore
    // ~   ?                   <  >  {  }  |
    // tab                           :  "
    //                                  =
    //                         ,

    // [C128 SHIFTED]
    // stop !   @ # $ % ↑ & *  (  )  _  =  ins
    // ~    !   @ # $ % ↑ & *  (  )  <  >  |
    //      q   w e r t y u i  o  p  :  "  ins
    //          a s d f g h j  k  l  up ←  return  SHF = SHIFT
    //          z x c v b n m  .  lt dn rt space

    // special keys
    // SCAN_CODE_CAPS = 128,
	// SCAN_CODE_DISPLAY = 256,
	// SCAN_CODE_FLAG_RESTORE = 1024,
	// SCAN_CODE_FLAG_FORCE_SHIFT = 2048,
	// SCAN_CODE_FLAG_FORCE_NOSHIFT = 4096,
	// SCAN_CODE_FLAG_FORCE_COMMODORE = 8192,

    uint16_t to128[3][70] = {
      { 63, 56, 59,  8, 11, 16, 19, 24, 27, 32, 35, 43, 40,  0,
        17+8192, 56+2048, 46,  8+2048, 11+2048, 16+2048, 54, 19+2048, 49, 27+2048, 32+2048, 45+2048, 50+2048, 48,
        61, 62,  9, 14, 17, 22, 25, 30, 33, 38, 41, 50, 24+2048,  0,
        88, 15, 10, 13, 18, 21, 26, 29, 34, 37, 42, 83, 57,  1,
        58, 80, 12, 23, 20, 31, 28, 39, 36, 44, 85, 84, 86, 60 },

      { 63, 56, 46+4096,  8, 11, 16, 54+4096, 19, 49+4096, 27, 32, 57+4096, 53+4096,  0,
        14+4096+8192, 55, 46+4096,  8, 11, 16, 54, 19, 55+4096, 47, 44, 62+4096+8192, 9+4096+8192, 43+2048,
        61, 62,  9, 14, 17, 22, 25, 30, 33, 38, 41, 45+4096, 59, 0,
        88, 15, 10, 13, 18, 21, 26, 29, 34, 37, 42, 83+4096, 53+4096,  1,
        58, 80, 12, 23, 20, 31, 28, 39, 36, 47+4096, 85+4096, 84+4096, 86+4096, 60 },

      { 72, 4, 4+2048, 5, 5+2048, 6, 6+2048, 3, 3+2048, 88, 88, 88, 51, 1024, 
        14+8192, 55+2048, 88, 88, 88, 88, 88, 88, 55, 47+2048, 44+2048, 62+8192, 9+8192, 43+2048,
        67, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 45, 59+2048, 88, 
        88, 15, 88, 88, 88, 88, 88, 88, 88, 88, 88, 88, 53, 88, 
        88, 88, 88, 88, 88, 88, 88, 88, 88, 47, 88, 88, 88, 88 }
    };

    static uint16_t c128_keys[8];

    void insert_key(uint16_t key, int &count)
    {
        if (count < 0 || count >= 8)
            return;
        for (int i=count-1; i>0; --i)
            c128_keys[i] = c128_keys[i-1];
        c128_keys[0] = key;
        ++count;
    }

    void delete_first_key(int &count)
    {
        if (count < 1 || count > 8)
            return;
        for (int i=1; i<count-1; ++i)
            c128_keys[i-1] = c128_keys[i];
        --count;
    }
}

void tab5_key_matrix_to_c128(const m5::unit::tab5_keyboard::key_status_bits_t &matrix, uint16_t *&c128_keys, uint8_t &c128_size)
{
    if (matrix.size() != 70)
        return;

    bool force_shift = false;
    bool force_noshift = false;
    bool force_cbm = false;
    bool lshift = matrix[3*14 + 1] == 1;
    bool cbm = matrix[2*14] == 1;
    bool sym = matrix[3*14] == 1;
    bool ctrl = matrix[4*14] == 1;
    int index = sym ? 2 : (lshift ? 1 : 0);

    c128_keys = ::c128_keys;
    c128_size = 8;

    int count = 0;

    if (lshift)
        c128_keys[count++] = to128[index][3*14 + 1];

    for (int i=0; i<70; ++i) 
    {
        if (i == 3*14 + 1)
            continue;
        if (matrix[i] == 0)
            continue;
        auto c128key = to128[index][i];
        if (c128key == 88)
            continue;
        if ((c128key & 2048) != 0 && !lshift && !force_shift) {
            if (count > 6)
                continue;
            force_shift = true;
            insert_key(15, count);
        }            
        if ((c128key & 4096) != 0 && !force_noshift && lshift) {
            if (force_shift || count != 1)
                continue;
            delete_first_key(count);
            force_noshift = true;
        }            
        if ((c128key & 8192) != 0 && !force_cbm && !cbm) {
            if (count > 6)
                continue;
            force_cbm = true;
            insert_key(61, count);
        }
        c128_keys[count++] = c128key & 0x5ff;
        if (c128key == 1024 && ctrl && count == 1)
            c128_keys[count++] = 63;          
        if (count == 8)
            break;
    }

    while (count < 8)
        c128_keys[count++] = 88;
}
