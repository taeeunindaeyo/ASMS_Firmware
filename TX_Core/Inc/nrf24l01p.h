#ifndef __NRF24L01P_H
#define __NRF24L01P_H

#include "stm32f1xx.h"
#include <stdint.h>

#define NRF_CONFIG      0x00
#define NRF_EN_AA       0x01
#define NRF_EN_RXADDR   0x02
#define NRF_SETUP_AW    0x03
#define NRF_SETUP_RETR  0x04
#define NRF_RF_CH       0x05
#define NRF_RF_SETUP    0x06
#define NRF_STATUS      0x07
#define NRF_TX_ADDR     0x10
#define NRF_RX_ADDR_P0  0x0A
#define NRF_DYNPD       0x1C
#define NRF_FEATURE     0x1D

#define NRF_R_RX_PL_WID 0x60
#define NRF_R_RX_PAYLOAD 0x61
#define NRF_W_TX_PAYLOAD 0xA0
#define NRF_FLUSH_TX    0xE1
#define NRF_FLUSH_RX    0xE2
#define NRF_ACTIVATE    0x50

// 설정값 (초기화와 확인에 같이 사용)
#define NRF_TX_CONFIG_VAL  0x0A   // PWR_UP, EN_CRC, PTX
#define NRF_CHANNEL        76

uint8_t nRF_Init_TX(uint8_t tries);   // 반환: 1=설정 확인 완료, 0=실패
uint8_t nRF_CheckConfig_TX(void);     // 반환: 1=레지스터가 설정값 그대로
uint8_t nRF_TxPacket(uint8_t *tx_buf, uint8_t *ack_buf);

void nRF_WriteReg(uint8_t reg, uint8_t value);
uint8_t nRF_ReadReg(uint8_t reg);
void nRF_WriteBuf(uint8_t reg, uint8_t *pBuf, uint8_t len);
void nRF_ReadBuf(uint8_t reg, uint8_t *pBuf, uint8_t len);
void nRF_Command(uint8_t cmd);

#endif
