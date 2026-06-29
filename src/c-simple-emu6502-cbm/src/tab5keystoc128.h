#include <Arduino.h>
#include <M5UnitUnifiedKEYBOARD.h>
extern void tab5_key_matrix_to_c128(const m5::unit::tab5_keyboard::key_status_bits_t &matrix, uint16_t *&c128_keys, uint8_t &c128_size);
