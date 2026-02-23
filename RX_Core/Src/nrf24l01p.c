#include "nrf24l01p.h"
#include "gpio.h"
#include "spi.h"
#include "delay.h"

static uint8_t ADDR[5] = {0xB0, 0xB1, 0xB2, 0xB3, 0xB4};

void nRF_Init_RX(void) {
    CE_LOW();
    nRF_WriteReg(NRF_CONFIG, 0x0B);      // RX Mode, Power Up
    Delay_ms(2);

    nRF_WriteReg(NRF_EN_AA, 0x02);       // Pipe 1 Auto-ACK
    nRF_WriteReg(NRF_EN_RXADDR, 0x02);   // Pipe 1 Enable
    nRF_WriteReg(NRF_SETUP_AW, 0x03);    // 5 Bytes Address
    nRF_WriteReg(NRF_RF_CH, 76);         // Channel 76
    nRF_WriteReg(NRF_RF_SETUP, 0x0E);    // 2Mbps

    // Dynamic Payload & ACK Payload 활성화
    nRF_WriteReg(NRF_FEATURE, 0x06);     // EN_DPL, EN_ACK_PAY
    nRF_WriteReg(NRF_DYNPD, 0x03);       // Pipe 0, 1 Dynamic

    nRF_WriteBuf(NRF_RX_ADDR_P1, ADDR, 5);

    CE_HIGH(); // 수신 대기 시작
}

void nRF_LoadAckPayload(uint8_t *data, uint8_t len) {
    // Pipe 1번에 답장 장전 (0xA8 | 0x01 = 0xA9)
    CSN_LOW();
    SPI1_Transfer(0xA9);
    for(int i=0; i<len; i++) SPI1_Transfer(data[i]);
    CSN_HIGH();
}

void nRF_ReceivePacket(uint8_t *rx_buf) {
    CSN_LOW();
    SPI1_Transfer(NRF_R_RX_PAYLOAD);
    for(int i=0; i<32; i++) rx_buf[i] = SPI1_Transfer(0xFF);
    CSN_HIGH();

    nRF_WriteReg(NRF_FLUSH_RX, 0xFF);
}

void nRF_WriteReg(uint8_t reg, uint8_t value) {
    CSN_LOW(); SPI1_Transfer(reg | 0x20); SPI1_Transfer(value); CSN_HIGH();
}
uint8_t nRF_ReadReg(uint8_t reg) {
    uint8_t val; CSN_LOW(); SPI1_Transfer(reg & 0x1F); val = SPI1_Transfer(0xFF); CSN_HIGH(); return val;
}
void nRF_WriteBuf(uint8_t reg, uint8_t *b, uint8_t l) {
    CSN_LOW(); SPI1_Transfer(reg | 0x20); for(int i=0; i<l; i++) SPI1_Transfer(b[i]); CSN_HIGH();
}
