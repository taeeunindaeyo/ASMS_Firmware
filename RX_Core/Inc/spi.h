#ifndef __SPI_H
#define __SPI_H

#include "stm32f1xx.h"
#include <stdint.h>

void SPI1_Init(void);
uint8_t SPI1_Transfer(uint8_t data);

#endif