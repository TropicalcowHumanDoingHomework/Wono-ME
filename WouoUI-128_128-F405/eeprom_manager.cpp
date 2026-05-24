#include "eeprom_manager.h"
#include "ui_state.h"
#include "EEPROM.h"

/************************************* 断电保存 *************************************/

//EEPROM变量
EepromState eeprom;

// 验证单个数值是否在范围内，否则使用默认值
static uint8_t validate_param(uint8_t value, uint8_t min_val, uint8_t max_val, uint8_t default_val) {
  if (value < min_val || value > max_val) {
    return default_val;
  }
  return value;
}

// 验证所有UI参数
static void validate_ui_params() {
  // 先保存从EEPROM读取的值
  uint8_t eeprom_ui_params[UI_PARAM];
  for (uint8_t i = 0; i < UI_PARAM; ++i) {
    eeprom_ui_params[i] = ui.param[i];
  }
  
  // 加载默认值作为参考
  ui_param_init();
  
  //验证每个参数，EEPROM值有效则使用，否则使用默认值
  ui.param[DISP_BRI] = validate_param(eeprom_ui_params[DISP_BRI], 0, 1, ui.param[DISP_BRI]);
  ui.param[TILE_ANI] = validate_param(eeprom_ui_params[TILE_ANI], 10, 100, ui.param[TILE_ANI]);
  ui.param[LIST_CUR] = validate_param(eeprom_ui_params[LIST_CUR], 0, 200, ui.param[LIST_CUR]);
  ui.param[BOX_X_OS] = validate_param(eeprom_ui_params[BOX_X_OS], 0, 40, ui.param[BOX_X_OS]);
  ui.param[BOX_Y_OS] = validate_param(eeprom_ui_params[BOX_Y_OS], 0, 40, ui.param[BOX_Y_OS]);
  ui.param[WIN_Y_OS] = validate_param(eeprom_ui_params[WIN_Y_OS], 0, 40, ui.param[WIN_Y_OS]);
  ui.param[LIST_ANI] = validate_param(eeprom_ui_params[LIST_ANI], 10, 100, ui.param[LIST_ANI]);
  ui.param[WIN_ANI] = validate_param(eeprom_ui_params[WIN_ANI], 10, 100, ui.param[WIN_ANI]);
  ui.param[SPOT_ANI] = validate_param(eeprom_ui_params[SPOT_ANI], 10, 100, ui.param[SPOT_ANI]);
  ui.param[TAG_ANI] = validate_param(eeprom_ui_params[TAG_ANI], 10, 100, ui.param[TAG_ANI]);
  ui.param[FADE_ANI] = validate_param(eeprom_ui_params[FADE_ANI], 0, 255, ui.param[FADE_ANI]);
  ui.param[BTN_SPT] = validate_param(eeprom_ui_params[BTN_SPT], 0, 255, ui.param[BTN_SPT]);
  ui.param[BTN_LPT] = validate_param(eeprom_ui_params[BTN_LPT], 0, 255, ui.param[BTN_LPT]);
  ui.param[TILE_UFD] = validate_param(eeprom_ui_params[TILE_UFD], 0, 1, ui.param[TILE_UFD]);
  ui.param[LIST_UFD] = validate_param(eeprom_ui_params[LIST_UFD], 0, 1, ui.param[LIST_UFD]);
  ui.param[TILE_LOOP] = validate_param(eeprom_ui_params[TILE_LOOP], 0, 1, ui.param[TILE_LOOP]);
  ui.param[LIST_LOOP] = validate_param(eeprom_ui_params[LIST_LOOP], 0, 1, ui.param[LIST_LOOP]);
  ui.param[WIN_BOK] = validate_param(eeprom_ui_params[WIN_BOK], 0, 1, ui.param[WIN_BOK]);
  ui.param[KNOB_DIR] = validate_param(eeprom_ui_params[KNOB_DIR], 0, 1, ui.param[KNOB_DIR]);
  ui.param[DARK_MODE] = validate_param(eeprom_ui_params[DARK_MODE], 0, 1, ui.param[DARK_MODE]);
  ui.param[ROTATE_SCR] = validate_param(eeprom_ui_params[ROTATE_SCR], 0, 3, ui.param[ROTATE_SCR]);
  ui.param[BUZ_VOL] = validate_param(eeprom_ui_params[BUZ_VOL], 0, 4, ui.param[BUZ_VOL]);
  ui.param[USB_ENABLE] = validate_param(eeprom_ui_params[USB_ENABLE], 0, 1, ui.param[USB_ENABLE]);
  ui.param[WIN_STYLE] = validate_param(eeprom_ui_params[WIN_STYLE], 0, 1, ui.param[WIN_STYLE]);
  ui.param[FADE_MODE] = validate_param(eeprom_ui_params[FADE_MODE], 0, 1, ui.param[FADE_MODE]);
  ui.param[HL_ANI_MODE] = validate_param(eeprom_ui_params[HL_ANI_MODE], 0, 2, ui.param[HL_ANI_MODE]);
  ui.param[SPRING_K] = validate_param(eeprom_ui_params[SPRING_K], 10, 100, ui.param[SPRING_K]);
  ui.param[SPRING_D] = validate_param(eeprom_ui_params[SPRING_D], 10, 100, ui.param[SPRING_D]);
}

// 验证所有旋钮参数
static void validate_knob_params() {
  // 旋钮参数一般是0或1等小范围值
  for (uint8_t i = 0; i < KNOB_PARAM; ++i) {
    knob.param[i] = validate_param(knob.param[i], 0, 255, 0);
  }
}

//EEPROM写数据，回到睡眠时执行一遍
void eeprom_write_all_data()
{
  eeprom.address = 0;
  for (uint8_t i = 0; i < EEPROM_CHECK; ++i)    EEPROM.write(eeprom.address + i, eeprom.check_param[i]);  eeprom.address += EEPROM_CHECK;
  for (uint8_t i = 0; i < UI_PARAM; ++i)        EEPROM.write(eeprom.address + i, ui.param[i]);            eeprom.address += UI_PARAM;
  for (uint8_t i = 0; i < KNOB_PARAM; ++i)      EEPROM.write(eeprom.address + i, knob.param[i]);          eeprom.address += KNOB_PARAM;
  EEPROM.commit();
}

//EEPROM读数据，开机初始化时执行一遍
void eeprom_read_all_data()
{
  eeprom.address = EEPROM_CHECK;
  for (uint8_t i = 0; i < UI_PARAM; ++i)        ui.param[i] = EEPROM.read(eeprom.address + i);            eeprom.address += UI_PARAM;
  for (uint8_t i = 0; i < KNOB_PARAM; ++i)      knob.param[i] = EEPROM.read(eeprom.address + i);          eeprom.address += KNOB_PARAM;
  
  // 验证读取的数据是否在有效范围内
  validate_ui_params();
  validate_knob_params();
}

//开机检查是否已经修改过，没修改过则跳过读配置步骤，用默认设置
void eeprom_init()
{
  EEPROM.begin(512);
  eeprom.check = 0;
  eeprom.address = 0; for (uint8_t i = 0; i < EEPROM_CHECK; ++i)  if (EEPROM.read(eeprom.address + i) != eeprom.check_param[i])  eeprom.check++;
  if (eeprom.check <= 1) eeprom_read_all_data();  //允许一位误码
  else { ui_param_init(); eeprom_write_all_data(); }  //校验失败，初始化默认设置并保存
}
