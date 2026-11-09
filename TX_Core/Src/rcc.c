#include "rcc.h"

uint32_t g_sysclk_hz = 8000000UL;
uint32_t g_pclk1_hz  = 8000000UL;
uint32_t g_pclk2_hz  = 8000000UL;

void clock_init_safe(void)
{
    SystemClock_Config();
}

void SystemClock_Config(void)
{
    /*
     * 항상 HSI 8MHz에서 시작
     */
    RCC->CR |= RCC_CR_HSION;

    while ((RCC->CR & RCC_CR_HSIRDY) == 0U)
    {
    }

    /*
     * PLL을 설정하기 전에 SYSCLK를 HSI로 전환
     */
    RCC->CFGR =
        (RCC->CFGR & ~RCC_CFGR_SW) |
        RCC_CFGR_SW_HSI;

    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI)
    {
    }

    /*
     * PLL 정지
     */
    RCC->CR &= ~RCC_CR_PLLON;

    while ((RCC->CR & RCC_CR_PLLRDY) != 0U)
    {
    }

    /*
     * HSE 8MHz 크리스탈이 있는지 확인
     * 크리스탈이 없더라도 무한 대기하지 않음
     */
    RCC->CR |= RCC_CR_HSEON;

    uint32_t timeout = 2000000UL;

    while (((RCC->CR & RCC_CR_HSERDY) == 0U) &&
           (timeout != 0U))
    {
        timeout--;
    }

    uint32_t hse_ready =
        RCC->CR & RCC_CR_HSERDY;

    /*
     * 64/72MHz 동작을 위한 Flash 설정
     */
    FLASH->ACR =
        (FLASH->ACR & ~FLASH_ACR_LATENCY) |
        FLASH_ACR_PRFTBE |
        FLASH_ACR_LATENCY_2;

    /*
     * 기존 클럭 설정 제거
     */
    RCC->CFGR &=
        ~(RCC_CFGR_HPRE |
          RCC_CFGR_PPRE1 |
          RCC_CFGR_PPRE2 |
          RCC_CFGR_PLLSRC |
          RCC_CFGR_PLLXTPRE |
          RCC_CFGR_PLLMULL);

    /*
     * AHB  = SYSCLK / 1
     * APB1 = HCLK / 2
     * APB2 = HCLK / 1
     */
    RCC->CFGR |=
        RCC_CFGR_HPRE_DIV1 |
        RCC_CFGR_PPRE1_DIV2 |
        RCC_CFGR_PPRE2_DIV1;

    if (hse_ready != 0U)
    {
        /*
         * 외부 크리스탈 존재
         *
         * HSE 8MHz × 9 = 72MHz
         * PCLK1 = 36MHz
         * PCLK2 = 72MHz
         */
        RCC->CFGR |=
            RCC_CFGR_PLLSRC |
            RCC_CFGR_PLLMULL9;

        g_sysclk_hz = 72000000UL;
        g_pclk1_hz  = 36000000UL;
        g_pclk2_hz  = 72000000UL;
    }
    else
    {
        /*
         * 외부 크리스탈 없음
         *
         * HSI 8MHz / 2 × 16 = 64MHz
         * PCLK1 = 32MHz
         * PCLK2 = 64MHz
         */
        RCC->CR &= ~RCC_CR_HSEON;

        RCC->CFGR |= RCC_CFGR_PLLMULL16;

        g_sysclk_hz = 64000000UL;
        g_pclk1_hz  = 32000000UL;
        g_pclk2_hz  = 64000000UL;
    }

    /*
     * PLL 활성화
     */
    RCC->CR |= RCC_CR_PLLON;

    while ((RCC->CR & RCC_CR_PLLRDY) == 0U)
    {
    }

    /*
     * SYSCLK 소스를 PLL로 변경
     */
    RCC->CFGR =
        (RCC->CFGR & ~RCC_CFGR_SW) |
        RCC_CFGR_SW_PLL;

    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL)
    {
    }

    /*
     * CMSIS에서 사용하는 코어 클럭 변수 갱신
     */
    SystemCoreClock = g_sysclk_hz;
}
