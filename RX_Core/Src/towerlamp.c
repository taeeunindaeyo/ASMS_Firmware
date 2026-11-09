#include "towerlamp.h"

void towerlamp_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    // PB12, PB13, PB14 설정 제거
    GPIOB->CRH &= ~(
        (0xFU << ((12 - 8) * 4)) |
        (0xFU << ((13 - 8) * 4)) |
        (0xFU << ((14 - 8) * 4))
    );

    // 2MHz Push-pull 출력
    GPIOB->CRH |= (
        (0x2U << ((12 - 8) * 4)) |
        (0x2U << ((13 - 8) * 4)) |
        (0x2U << ((14 - 8) * 4))
    );

    towerlamp_all_off();
}

void towerlamp_all_off(void)
{
    RLED_OFF();
    YLED_OFF();
    BLED_OFF();
}
