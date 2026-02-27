#include "towerlamp.h"

void towerlamp_init(void)
{

    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    // 초기화(해당 12비트 clear)
    GPIOA->CRH &= ~(
        (0xFU << ((8  - 8) * 4)) |   // PA8
        (0xFU << ((9  - 8) * 4)) |   // PA9
        (0xFU << ((10 - 8) * 4))     // PA10
    );

    // 출력 모드 설정
    GPIOA->CRH |= (
        (0x2U << ((8  - 8) * 4)) |
        (0x2U << ((9  - 8) * 4)) |
        (0x2U << ((10 - 8) * 4))
    );

    towerlamp_all_off();
}

void towerlamp_all_off(void)
{
    RLED_OFF();
    YLED_OFF();
    BLED_OFF();
}
