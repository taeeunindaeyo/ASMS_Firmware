#include "common.h"
#include "rcc.h"

#define USART1_BAUDRATE 115200UL

void USART1_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    /*
     * PA9  : USART1 TX
     * PA10 : USART1 RX
     */
    GPIOA->CRH &= 0xFFFFFF0F;
    GPIOA->CRH |= 0x000004B0;

    /*
     * 실제 PCLK2를 이용해 BRR 자동 계산
     *
     * 72MHz일 때 약 0x271
     * 64MHz일 때 약 0x22C
     */
    USART1->BRR =
        (g_pclk2_hz + (USART1_BAUDRATE / 2UL)) /
        USART1_BAUDRATE;

    USART1->CR1 |=
        USART_CR1_UE |
        USART_CR1_TE |
        USART_CR1_RE;
}

void USART1_SendByte(uint8_t b)
{
    while ((USART1->SR & USART_SR_TXE) == 0U)
    {
    }

    USART1->DR = b;
}

void USART1_SendString(char *str)
{
    while (*str != '\0')
    {
        USART1_SendByte((uint8_t)*str);
        str++;
    }
}
