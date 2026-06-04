#ifndef SIM_EEPROM_H
#define SIM_EEPROM_H

#include "ui_types.h"

extern EepromState eeprom;

void eeprom_write_all_data();
void eeprom_read_all_data();
void eeprom_init();

#endif
