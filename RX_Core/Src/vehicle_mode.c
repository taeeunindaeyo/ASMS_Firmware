#include "vehicle_mode.h"

Mode vehicle_decide_mode(uint8_t a, uint8_t m, uint8_t e)
{
    if (e) return MODE_ESTOP;
    if (m) return MODE_MANUAL;
    if (a) return MODE_AUTO;
    return MODE_NONE;
}
