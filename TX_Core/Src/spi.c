#include "spi.h"

void SPI1_Init(void) {
		RCC->APB2ENR |= (RCC_APB2ENR_SPI1EN | RCC_APB2ENR_AFIOEN);
		
		SPI1->CR1 = (1<<9)|(1<<8)|(1<<6)|(1<<2)|(2<<3);
		SPI1->CR2 = 0;
		
		SPI1->CR1 |= (1<<6); // SPI 활성화
}

uint8_t SPI1_Transfer(uint8_t data) {
		SPI1->DR = data;
		while(!(SPI1->SR & SPI_SR_RXNE)); // until rx buffer full
		return SPI1->DR;
}