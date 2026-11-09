#ifndef __RCC_H
#define __RCC_H

#include "stm32f1xx.h"

extern uint32_t g_sysclk_hz;
extern uint32_t g_pclk1_hz;
extern uint32_t g_pclk2_hz;

void clock_init_safe(void);
void SystemClock_Config(void);

#endif
