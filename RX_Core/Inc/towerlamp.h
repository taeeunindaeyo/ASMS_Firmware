#ifndef TOWERLAMP_H_
#define TOWERLAMP_H_

#include "stm32f1xx.h"

// RED: PB12, YELLOW: PB13, BLUE: PB14

#define RLED_ON()   (GPIOB->BSRR = (1U << 12))
#define RLED_OFF()  (GPIOB->BSRR = (1U << (12 + 16)))

#define YLED_ON()   (GPIOB->BSRR = (1U << 13))
#define YLED_OFF()  (GPIOB->BSRR = (1U << (13 + 16)))

#define BLED_ON()   (GPIOB->BSRR = (1U << 14))
#define BLED_OFF()  (GPIOB->BSRR = (1U << (14 + 16)))

void towerlamp_init(void);
void towerlamp_all_off(void);

#endif
