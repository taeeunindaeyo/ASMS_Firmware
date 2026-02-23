#ifndef INC_SW_HANDLER_H_
#define INC_SW_HANDLER_H_

#include <stdint.h>

#define SW_AUTO_PIN    13
#define SW_MANUAL_PIN  14
#define SW_ESTOP_PIN   15

typedef struct {
    uint8_t stable;
    uint8_t last;
    uint8_t cnt;
} Deb;

void    sw_init_pullup(void);
uint8_t sw_raw_active_low(uint8_t pin);   // 눌림=1, 안눌림=0
uint8_t deb_update(Deb *d, uint8_t raw);

#endif
