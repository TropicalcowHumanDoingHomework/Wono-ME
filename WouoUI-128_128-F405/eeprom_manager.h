#ifndef EEPROM_MANAGER_H
#define EEPROM_MANAGER_H

#include "ui_types.h"

/************************************* 断电保存 *************************************/

//EEPROM状态变量：管理断电存储的数据校验和写入状态
extern EepromState eeprom;

//EEPROM写数据：将所有配置参数写入Flash模拟EEPROM，回到睡眠时执行
void eeprom_write_all_data();

//EEPROM读数据：从Flash模拟EEPROM读取所有配置参数，开机初始化时执行
void eeprom_read_all_data();

//EEPROM初始化：开机检查校验状态，有效则读取配置，无效则使用默认设置
void eeprom_init();

#endif
