#ifndef __GPIO_H
#define __GPIO_H

#include "stm32f1xx.h"

#define CSN_LOW()     (GPIOA->BRR = GPIO_BRR_BR4)
#define CSN_HIGH()    (GPIOA->BSRR = GPIO_BSRR_BS4)

#define CE_LOW()      (GPIOA->BRR = GPIO_BRR_BR3)
#define CE_HIGH()     (GPIOA->BSRR = GPIO_BSRR_BS3)

#define LED_ON()      (GPIOC->BRR = GPIO_BRR_BR13)
#define LED_OFF()     (GPIOC->BSRR = GPIO_BSRR_BS13)

void GPIO_Init(void);

void blink_fast(int n);

#endif