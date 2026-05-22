#ifndef HW_ABSTRACTION_H
#define HW_ABSTRACTION_H

#include <stdint.h>

void hw_Init(void);
uint32_t hw_Millis(void);

/* 引脚操作宏(K数组,使用sys.h的位带操作) */
#define KNOB_A     PBout(12)
#define KNOB_B     PBout(13)
#define KNOB_SW    PBout(14)

#define KNOB_A_IN  PBin(12)
#define KNOB_B_IN  PBin(13)
#define KNOB_SW_IN PBin(14)

#define KNOB_A_H() PBout(12)=1
#define KNOB_A_L() PBout(12)=0
#define KNOB_B_H() PBout(13)=1
#define KNOB_B_L() PBout(13)=0
#define KNOB_SW_H() PBout(14)=1
#define KNOB_SW_L() PBout(14)=0

#endif
