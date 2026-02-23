#include "i2c.h"
#include "stm32f1xx.h"

// main.c 쪽에 있는 delay_ms를 사용 (중복 정의 방지)
extern void Delay_ms(uint32_t ms);

void i2c1_init(uint32_t i2c_hz, uint32_t pclk1_hz)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    // PB6/PB7: AF Open-drain 50MHz (CNF=11, MODE=11 => 0xF)
    GPIOB->CRL &= ~((0xF << (6*4)) | (0xF << (7*4)));
    GPIOB->CRL |=  ((0xF << (6*4)) | (0xF << (7*4)));

    I2C1->CR1 &= ~I2C_CR1_PE;

    // software reset
    I2C1->CR1 |= I2C_CR1_SWRST;
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    uint32_t pclk1_mhz = pclk1_hz / 1000000UL;

    // CR2 = PCLK1(MHz)
    I2C1->CR2 = (uint16_t)pclk1_mhz;

    // Standard mode: CCR = PCLK1/(2*Fscl)
    uint32_t ccr = pclk1_hz / (2UL * i2c_hz);
    if (ccr < 4) ccr = 4;
    I2C1->CCR = (uint16_t)ccr;

    // TRISE = PCLK1(MHz) + 1
    I2C1->TRISE = (uint16_t)(pclk1_mhz + 1);

    I2C1->CR1 |= I2C_CR1_PE;
}

int i2c1_write(uint8_t addr7, const uint8_t *buf, uint16_t len)
{
    uint32_t to;

    // BUSY면 STOP 시도
    if (I2C1->SR2 & I2C_SR2_BUSY) {
        I2C1->CR1 |= I2C_CR1_STOP;
        Delay_ms(1);
    }

    // START
    I2C1->CR1 |= I2C_CR1_START;
    to = 200000;
    while (((I2C1->SR1 & I2C_SR1_SB) == 0) && to--) {}
    if (!to) return -1;
    (void)I2C1->SR1;

    // ADDR (write)
    I2C1->DR = (addr7 << 1);
    to = 200000;
    while (((I2C1->SR1 & I2C_SR1_ADDR) == 0) && to--) {
        if (I2C1->SR1 & I2C_SR1_AF) { // NACK
            I2C1->SR1 &= ~I2C_SR1_AF;
            I2C1->CR1 |= I2C_CR1_STOP;
            return -2;
        }
    }
    if (!to) {
        I2C1->CR1 |= I2C_CR1_STOP;
        return -3;
    }
    (void)I2C1->SR1;
    (void)I2C1->SR2;

    // DATA
    for (uint16_t i = 0; i < len; i++) {
        to = 200000;
        while (((I2C1->SR1 & I2C_SR1_TXE) == 0) && to--) {}
        if (!to) {
            I2C1->CR1 |= I2C_CR1_STOP;
            return -4;
        }
        I2C1->DR = buf[i];
    }

    // BTF then STOP
    to = 200000;
    while (((I2C1->SR1 & I2C_SR1_BTF) == 0) && to--) {}
    I2C1->CR1 |= I2C_CR1_STOP;

    return to ? 0 : -5;
}

int i2c1_ping(uint8_t addr7)
{
    // SSD1306는 "control byte(0x00)" 하나만 보내도 ACK로 존재 확인 가능
    uint8_t pkt = 0x00;
    return i2c1_write(addr7, &pkt, 1);
}
