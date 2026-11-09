#include "delay.h"
#include "stm32f1xx.h"
#include "rcc.h"

static void DWT_Delay_Init(void)
{
    /*
     * Cortex-M3 DWT cycle counter 활성화
     */
    CoreDebug->DEMCR |=
        CoreDebug_DEMCR_TRCENA_Msk;

    DWT->CYCCNT = 0U;

    DWT->CTRL |=
        DWT_CTRL_CYCCNTENA_Msk;
}

void Delay_us(uint32_t us)
{
    /*
     * 처음 사용할 때 DWT 초기화
     */
    if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0U)
    {
        DWT_Delay_Init();
    }

    uint32_t start = DWT->CYCCNT;

    uint32_t cycles =
        (g_sysclk_hz / 1000000UL) * us;

    while ((uint32_t)(DWT->CYCCNT - start) < cycles)
    {
    }
}

void Delay_ms(uint32_t ms)
{
    while (ms != 0U)
    {
        Delay_us(1000U);
        ms--;
    }
}
