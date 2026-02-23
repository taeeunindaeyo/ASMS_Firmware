#include "rcc.h"

uint32_t g_sysclk_hz = 8000000UL;
uint32_t g_pclk1_hz  = 8000000UL;

void clock_init_safe(void)
{
    RCC->CR |= RCC_CR_HSEON;
    uint32_t to = 2000000;
    while(((RCC->CR & RCC_CR_HSERDY) == 0) && to--) {}

    if(to == 0){
        g_sysclk_hz = 8000000UL;
        g_pclk1_hz  = 8000000UL;
        return;
    }

    FLASH->ACR |= FLASH_ACR_PRFTBE;
    FLASH->ACR &= ~FLASH_ACR_LATENCY;
    FLASH->ACR |= FLASH_ACR_LATENCY_2;

    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;

    RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL);
    RCC->CFGR |= (RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL9);

    RCC->CR |= RCC_CR_PLLON;
    while((RCC->CR & RCC_CR_PLLRDY) == 0) {}

    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}

    g_sysclk_hz = 72000000UL;
    g_pclk1_hz  = 36000000UL;
}

void SystemClock_Config(void) {
		RCC->CR |= RCC_CR_HSEON; // HSE ON
		while(!(RCC->CR & RCC_CR_HSERDY)); // 안정화 대기

		FLASH->ACR |= FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;
		RCC->CFGR |= (RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL9);

		RCC->CR |= RCC_CR_PLLON; // PLL ON
		while(!(RCC->CR & RCC_CR_PLLRDY)); // 안정화 대기

		RCC->CFGR |= RCC_CFGR_SW_PLL;
		while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}
