#include "gpio.h"

void GPIO_Init(void) {
		RCC->APB2ENR |= (RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN);
		
		GPIOA->CRL &= 0x000FFFFF;
		GPIOA->CRL |= 0xB8B00000;
		GPIOA->ODR |= (1<<6); // MISO 풀업
		
		GPIOA->CRL &= 0xFFF00FF0;
		GPIOA->CRL |= 0x00033000;
		
		GPIOC->CRH &= 0xFF0FFFFF;
		GPIOC->CRH |= 0x00300000;
		
		// 초기 상태 설정
		CSN_HIGH(); // SPI X
		CE_LOW(); // TX Stby
		LED_OFF();
}

void blink_fast(int n) {
    for(int i=0; i<n; i++){
        LED_ON();
        Delay_ms(200); // busy_delay 대신 태은님 Delay 사용
        LED_OFF();
        Delay_ms(200);
    }
}