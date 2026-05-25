#ifndef EEPROM_MANAGER_H
#define EEPROM_MANAGER_H

#include "ui_types.h"
#include <EEPROM.h>

/************************************* 断电保存 *************************************/

//EEPROM变量
extern EepromState eeprom;

//EEPROM写数据，回到睡眠时执行一遍
void eeprom_write_all_data();
//EEPROM读数据，开机初始化时执行一遍
void eeprom_read_all_data();
//开机检查是否已经修改过，没修改过则跳过读配置步骤，用默认设置
void eeprom_init();

#endif