#include "common.h"

void USART1_Init(void) {
		RCC->APB2ENR |= RCC_APB2ENR_USART1EN; // 클럭 활성화
		GPIOA->CRH &= 0xFFFFFF0F;
		GPIOA->CRH |= 0x000004B0;
		
		USART1->BRR = 0x271; // Baudrate 115200
		USART1->CR1 |= 0x200C;
}

void USART1_SendByte(uint8_t b) {
		while(!(USART1->SR & USART_SR_TXE)); // until tx buffer empty
		USART1->DR = b;
}

void USART1_SendString(char* str) {
		while(*str) USART1_SendByte(*str++);
}