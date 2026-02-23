#include "vehicle_mode.h"
#include "oled.h"

Mode vehicle_decide_mode(uint8_t a, uint8_t m, uint8_t e)
{
    if (e) return MODE_ESTOP;
    if (m) return MODE_MANUAL;
    if (a) return MODE_AUTO;
    return MODE_NONE;
}

void vehicle_oled_show_mode(Mode md)
{
    // 2배 글씨는 y=16 -> page2, page3 사용
    oled_clear_pages(2, 3);

    if (md == MODE_ESTOP) oled_inverse(1);
    else                 oled_inverse(0);

    if (md == MODE_AUTO)        oled_write_centered_2x("AUTO",   16);
    else if (md == MODE_MANUAL) oled_write_centered_2x("MANUAL", 16);
    else if (md == MODE_ESTOP)  oled_write_centered_2x("ESTOP",  16);
    else                        oled_write_centered_2x("NONE",   16);
}
