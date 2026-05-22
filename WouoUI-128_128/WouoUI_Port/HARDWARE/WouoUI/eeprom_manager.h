#ifndef EEPROM_MANAGER_H
#define EEPROM_MANAGER_H

#include "ui_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern EepromState eeprom;

void eeprom_write_all_data();
void eeprom_read_all_data();
void eeprom_init();

#ifdef __cplusplus
}
#endif

#endif