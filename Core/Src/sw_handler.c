#include "sw_handler.h"
#include "stm32f1xx.h"

void sw_init_pullup(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    // PB13/14/15 input pull-up/down (CNF=10, MODE=00) => 0x8
    GPIOB->CRH &= ~((0xF<<((13-8)*4)) | (0xF<<((14-8)*4)) | (0xF<<((15-8)*4)));
    GPIOB->CRH |=  ((0x8<<((13-8)*4)) | (0x8<<((14-8)*4)) | (0x8<<((15-8)*4)));

    // pull-up 선택: ODR=1
    GPIOB->ODR |= (1U<<13) | (1U<<14) | (1U<<15);
}

uint8_t sw_raw_active_low(uint8_t pin)
{
    // active-low: 눌리면 0 -> return 1
    return (GPIOB->IDR & (1U<<pin)) ? 0U : 1U;
}

uint8_t deb_update(Deb *d, uint8_t raw)
{
    // raw가 그대로 유지되면 cnt 증가 -> 일정 횟수 이상이면 stable 갱신
    if (raw == d->last) {
        if (d->cnt < 255) d->cnt++;
        // cnt 기준은 main에서 “몇 번 반복되면 안정”으로 쓸 거라,
        // 여기서는 그냥 stable 업데이트는 main이 설정한 임계치 방식으로 쓰기 좋게 유지
    } else {
        d->last = raw;
        d->cnt = 0;
    }
    return d->stable;
}
