#ifndef __RCC_H
#define __RCC_H

#include "stm32f1xx.h"

void clock_init_safe(void);

void SystemClock_Config(void);

#endif