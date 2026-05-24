#include "eeprom_manager.h"
#include "ui_state.h"

EepromState eeprom;

static uint8_t eeprom_ram[256];
static uint8_t eeprom_initialized = 0;

void eeprom_write_all_data(void) {
    uint8_t i;
    eeprom.address = 0;
    for (i = 0; i < EEPROM_CHECK; ++i)
        eeprom_ram[eeprom.address + i] = eeprom.check_param[i];
    eeprom.address += EEPROM_CHECK;
    for (i = 0; i < UI_PARAM; ++i)
        eeprom_ram[eeprom.address + i] = ui.param[i];
    eeprom.address += UI_PARAM;
    for (i = 0; i < KNOB_PARAM; ++i)
        eeprom_ram[eeprom.address + i] = knob.param[i];
    eeprom.address += KNOB_PARAM;
}

void eeprom_read_all_data(void) {
    uint8_t i;
    eeprom.address = EEPROM_CHECK;
    for (i = 0; i < UI_PARAM; ++i)
        ui.param[i] = eeprom_ram[eeprom.address + i];
    eeprom.address += UI_PARAM;
    for (i = 0; i < KNOB_PARAM; ++i)
        knob.param[i] = eeprom_ram[eeprom.address + i];
    eeprom.address += KNOB_PARAM;
}

void eeprom_init(void) {
    uint8_t i;
    /* 初始化EEPROM校验参数(Arduino原版: 'a'~'k') */
    static const uint8_t default_check[EEPROM_CHECK] = {'a','b','c','d','e','f','g','h','i','j','k'};
    memcpy(eeprom.check_param, default_check, EEPROM_CHECK);

    if (!eeprom_initialized) {
        memset(eeprom_ram, 0, sizeof(eeprom_ram));
        eeprom_initialized = 1;
        ui_param_init();
        return;
    }
    eeprom.check = 0;
    eeprom.address = 0;
    for (i = 0; i < EEPROM_CHECK; ++i)
        if (eeprom_ram[eeprom.address + i] != eeprom.check_param[i])
            eeprom.check++;
    if (eeprom.check <= 1) {
        eeprom_read_all_data();
    } else {
        ui_param_init();
    }
}
