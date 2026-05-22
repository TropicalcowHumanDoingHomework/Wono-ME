#include "eeprom_manager.h"
#include "ui_state.h"

/************************************* 断电保存 *************************************/

//EEPROM变量
EepromState eeprom;

//EEPROM写数据，回到睡眠时执行一遍
void eeprom_write_all_data() {
    eeprom.address = 0;
    for (uint8_t i = 0; i < EEPROM_CHECK; ++i)
        EEPROM.write(eeprom.address + i, eeprom.check_param[i]);
    eeprom.address += EEPROM_CHECK;
    for (uint8_t i = 0; i < UI_PARAM; ++i)
        EEPROM.write(eeprom.address + i, ui.param[i]);
    eeprom.address += UI_PARAM;
    for (uint8_t i = 0; i < KNOB_PARAM; ++i)
        EEPROM.write(eeprom.address + i, knob.param[i]);
    eeprom.address += KNOB_PARAM;
}

//EEPROM读数据，开机初始化时执行一遍
void eeprom_read_all_data() {
    eeprom.address = EEPROM_CHECK;
    for (uint8_t i = 0; i < UI_PARAM; ++i)
        ui.param[i] = EEPROM.read(eeprom.address + i);
    eeprom.address += UI_PARAM;
    for (uint8_t i = 0; i < KNOB_PARAM; ++i)
        knob.param[i] = EEPROM.read(eeprom.address + i);
    eeprom.address += KNOB_PARAM;
}

//开机检查是否已经修改过，没修改过则跳过读配置步骤，用默认设置
void eeprom_init() {
    eeprom.check = 0;
    eeprom.address = 0;
    for (uint8_t i = 0; i < EEPROM_CHECK; ++i)
        if (EEPROM.read(eeprom.address + i) != eeprom.check_param[i])
            eeprom.check++;
    if (eeprom.check <= 1) {
        eeprom_read_all_data(); //允许一位误码
    } else {
        ui_param_init();
    }
}