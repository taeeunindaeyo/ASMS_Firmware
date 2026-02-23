#ifndef __COMMON_H
#define __COMMON_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "stm32f1xx.h"

/* --- UART 함수 --- */
void USART1_Init(void);
void USART1_SendByte(uint8_t b);
void USART1_SendString(char* str);

#endif