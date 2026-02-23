#ifndef INC_VEHICLE_MODE_H_
#define INC_VEHICLE_MODE_H_

#include <stdint.h>

typedef enum { MODE_NONE=0, MODE_AUTO, MODE_MANUAL, MODE_ESTOP } Mode;

Mode vehicle_decide_mode(uint8_t a, uint8_t m, uint8_t e);
void vehicle_oled_show_mode(Mode md);

#endif
