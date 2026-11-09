#include "nrf24l01p.h"
#include "gpio.h"
#include "spi.h"
#include "delay.h"

static uint8_t ADDR[5] = {0xB0, 0xB1, 0xB2, 0xB3, 0xB4};

// 레지스터 한 번 설정 (전원 다운 → FIFO·IRQ 정리 → 설정 → 전원 업). CE는 LOW로 끝남
static void nRF_Configure_RX(void) {
    CE_LOW();
    nRF_WriteReg(NRF_CONFIG, 0x09);      // Power Down (EN_CRC, PRIM_RX 유지)
    Delay_ms(2);

    nRF_Command(NRF_FLUSH_TX);           // 남아 있던 ACK 답장 정리
    nRF_Command(NRF_FLUSH_RX);           // 남아 있던 수신 패킷 정리
    nRF_WriteReg(NRF_STATUS, 0x70);      // RX_DR, TX_DS, MAX_RT 클리어

    nRF_WriteReg(NRF_EN_AA, 0x02);       // Pipe 1 Auto-ACK
    nRF_WriteReg(NRF_EN_RXADDR, 0x02);   // Pipe 1 Enable
    nRF_WriteReg(NRF_SETUP_AW, 0x03);    // 5 Bytes Address
    nRF_WriteReg(NRF_RF_CH, NRF_CHANNEL);// Channel 76
    nRF_WriteReg(NRF_RF_SETUP, 0x0E);    // 2Mbps

    // Dynamic Payload & ACK Payload 활성화
    nRF_WriteReg(NRF_FEATURE, 0x06);     // EN_DPL, EN_ACK_PAY
    if (nRF_ReadReg(NRF_FEATURE) != 0x06) {
        // 일부 nRF24L01(구형/호환칩)은 ACTIVATE 후에만 FEATURE가 써진다
        CSN_LOW(); SPI1_Transfer(NRF_ACTIVATE); SPI1_Transfer(0x73); CSN_HIGH();
        nRF_WriteReg(NRF_FEATURE, 0x06);
    }
    nRF_WriteReg(NRF_DYNPD, 0x03);       // Pipe 0, 1 Dynamic

    nRF_WriteBuf(NRF_RX_ADDR_P1, ADDR, 5);

    nRF_WriteReg(NRF_CONFIG, NRF_RX_CONFIG_VAL); // RX Mode, Power Up
    Delay_ms(5);                                 // Power Down → Standby 안정화 (1.5ms 이상)
}

// 써 놓은 값이 그대로 읽히는지 확인
uint8_t nRF_CheckConfig_RX(void) {
    uint8_t a[5];

    if (nRF_ReadReg(NRF_CONFIG)    != NRF_RX_CONFIG_VAL) return 0;
    if (nRF_ReadReg(NRF_EN_AA)     != 0x02) return 0;
    if (nRF_ReadReg(NRF_EN_RXADDR) != 0x02) return 0;
    if (nRF_ReadReg(NRF_RF_CH)     != NRF_CHANNEL) return 0;
    if (nRF_ReadReg(NRF_RF_SETUP)  != 0x0E) return 0;
    if (nRF_ReadReg(NRF_FEATURE)   != 0x06) return 0;
    if (nRF_ReadReg(NRF_DYNPD)     != 0x03) return 0;

    nRF_ReadBuf(NRF_RX_ADDR_P1, a, 5);
    for (int i = 0; i < 5; i++) if (a[i] != ADDR[i]) return 0;

    return 1;
}

// 설정 → 확인을 tries번까지 반복한 뒤 수신 대기 시작
uint8_t nRF_Init_RX(uint8_t tries) {
    uint8_t ok = 0;

    for (uint8_t t = 0; t < tries; t++) {
        nRF_Configure_RX();
        if (nRF_CheckConfig_RX()) { ok = 1; break; }
        Delay_ms(50);
    }

    CE_HIGH(); // 수신 대기 시작
    return ok;
}

void nRF_LoadAckPayload(uint8_t *data, uint8_t len) {
    // Pipe 1번에 답장 장전 (0xA8 | 0x01 = 0xA9)
    CSN_LOW();
    SPI1_Transfer(0xA9);
    for(int i=0; i<len; i++) SPI1_Transfer(data[i]);
    CSN_HIGH();
}

uint8_t nRF_ReceivePacket(uint8_t *rx_buf) {
    uint8_t len;

    // 패킷 실제 길이 확인
    CSN_LOW();
    SPI1_Transfer(NRF_R_RX_PL_WID);
    len = SPI1_Transfer(0xFF);
    CSN_HIGH();

    if (len == 0 || len > 32) {
        // 잘못된 길이일 때만 복구 목적으로 FIFO 비움
        CSN_LOW(); SPI1_Transfer(NRF_FLUSH_RX); CSN_HIGH();
        nRF_WriteReg(NRF_STATUS, 0x40);
        return 0;
    }

    CSN_LOW();
    SPI1_Transfer(NRF_R_RX_PAYLOAD);
    for(int i=0; i<len; i++) rx_buf[i] = SPI1_Transfer(0xFF);
    CSN_HIGH();

    nRF_WriteReg(NRF_STATUS, 0x40); // 읽은 직후 RX_DR 클리어
    return len;
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
void nRF_ReadBuf(uint8_t reg, uint8_t *b, uint8_t l) {
    CSN_LOW(); SPI1_Transfer(reg & 0x1F); for(int i=0; i<l; i++) b[i] = SPI1_Transfer(0xFF); CSN_HIGH();
}
// 데이터 바이트 없는 1바이트 명령 (FLUSH_TX, FLUSH_RX 등)
void nRF_Command(uint8_t cmd) {
    CSN_LOW(); SPI1_Transfer(cmd); CSN_HIGH();
}
