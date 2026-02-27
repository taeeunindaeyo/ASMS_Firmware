#ifndef towerlamp_H_
#define towerlamp_H_

#include "stm32f1xx.h"

// RED : PA8 , YELLOW : PA9 , BLUE : PA10

#define RLED_ON()     (GPIOA->BSRR = (1U << 9))
#define RLED_OFF()    (GPIOA->BSRR = (1U << (9  + 16)))

#define YLED_ON()     (GPIOA->BSRR = (1U << 10))
#define YLED_OFF()    (GPIOA->BSRR = (1U << (10  + 16)))

#define BLED_ON()     (GPIOA->BSRR = (1U << 11))
#define BLED_OFF()    (GPIOA->BSRR = (1U << (11 + 16)))

void towerlamp_init(void);
void towerlamp_all_off(void);

#endif
