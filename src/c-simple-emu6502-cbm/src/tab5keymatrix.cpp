#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedKEYBOARD.h>
#include <M5HAL.hpp>
#include <M5Utility.h>

#include "tab5keymatrix.h"

// This example targets UnitTab5Keyboard (Normal mode) only.
// `M5UnitUnifiedKEYBOARD.h` always includes `unit_Tab5Keyboard.hpp`, so no
// build-time define is required to opt into the Tab5 unit here.

namespace
{
    m5::unit::UnitUnified Units;
    m5::unit::UnitTab5Keyboard unit;

    // Tab5 Keyboard connects via ExtPort1 (10-pin internal connector) on M5Stack Tab5.
    // Pin assignment matches the SimpleDisplay example.
    //   INT = GPIO50 (J9 pin 10) -- handled by config_t default (irq_pin = 50)
    //   SDA = GPIO0  (J9 pin 7)
    //   SCL = GPIO1  (J9 pin 8)
    constexpr int8_t TAB5_KEYBOARD_SDA = 0;
    constexpr int8_t TAB5_KEYBOARD_SCL = 1;

    // Matrix geometry (5 rows x 14 cols) -- pulled from the unit's namespace constants
    // so the code automatically tracks any future change in KEY_COL_COUNT / KEY_COUNT.
    constexpr uint8_t MATRIX_ROWS = m5::unit::tab5_keyboard::KEY_COUNT / m5::unit::tab5_keyboard::KEY_COL_COUNT;
    constexpr uint8_t MATRIX_COLS = m5::unit::tab5_keyboard::KEY_COL_COUNT;

    bool setup_tab5_keyboard()
    {
        // Normal mode is required: this example reads bitwise per-key state, which is
        // only populated in Normal mode. INT pin defaults to GPIO50 (Tab5 ExtPort1).
        {
            auto cfg = unit.config();
            cfg.mode = m5::unit::tab5_keyboard::Mode::Normal;
            cfg.software_repeat = false;
            cfg.repeat_initial_ms = 1000;
            cfg.repeat_rate_ms = 1000;
            cfg.holding_threshold_ms = 500;
            unit.config(cfg);
        }

        //M5_LOGI("Tab5 ExtPort1 I2C: SDA:%d SCL:%d", TAB5_KEYBOARD_SDA, TAB5_KEYBOARD_SCL);
        Wire.end();
        Wire.begin(TAB5_KEYBOARD_SDA, TAB5_KEYBOARD_SCL, unit.component_config().clock);
        if (!Units.add(unit, Wire) || !Units.begin())
        {
            return false;
        }

        // begin() applies cfg.mode and (when start_periodic is true) enables the matching INT
        // and starts draining events, so no manual writeInterruptEnable()/startPeriodicMeasurement().
        //M5.Log.printf("Firmware:%02X\n", unit.firmwareVersion());
        return true;
    }

} // namespace

bool Tab5KeyMatrix::begin()
{
    if (!setup_tab5_keyboard())
        return false;
    // M5_LOGI("M5UnitUnified initialized");
    // M5_LOGI("%s", Units.debugInfo().c_str());
    return true;
}

void Tab5KeyMatrix::update()
{
    Units.update();
}

bool Tab5KeyMatrix::check_key_change(m5::unit::tab5_keyboard::key_status_bits_t &keys)
{
    auto prev = unit.previousBits();
    auto now = unit.nowBits();
    if (now == prev)
        return false;
    keys = now;
    return true;
}
