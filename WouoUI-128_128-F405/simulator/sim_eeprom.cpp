#include "sim_config.h"
#include "ui_types.h"
#include "ui_state.h"
#include <cstdio>
#include <cstring>

EepromState eeprom;

static constexpr const char* EEPROM_FILE = "wouo_params.bin";

void eeprom_write_all_data() {
    FILE* f = fopen(EEPROM_FILE, "wb");
    if (!f) return;
    fwrite(ui.param, 1, UI_PARAM, f);
    fwrite(knob.param, 1, KNOB_PARAM, f);
    fwrite(eeprom.check_param, 1, EEPROM_CHECK, f);
    fclose(f);
    eeprom.change = false;
}

void eeprom_read_all_data() {
    FILE* f = fopen(EEPROM_FILE, "rb");
    if (!f) {
        eeprom.check = 0;
        return;
    }
    fread(ui.param, 1, UI_PARAM, f);
    fread(knob.param, 1, KNOB_PARAM, f);
    fread(eeprom.check_param, 1, EEPROM_CHECK, f);
    fclose(f);
    eeprom.check = 1;
}

void eeprom_init() {
    eeprom.change = false;
    eeprom.address = 0;
    memset(eeprom.check_param, 0xAA, EEPROM_CHECK);
    eeprom_read_all_data();
    if (!eeprom.check) {
        ui_param_init();
    }
}
