#include <M5UnitUnifiedKEYBOARD.h>

class Tab5KeyMatrix {
public:
    static bool begin();
    static void update();
    static bool check_key_change(m5::unit::tab5_keyboard::key_status_bits_t &matrix);
private:
    Tab5KeyMatrix();
    ~Tab5KeyMatrix();
};
