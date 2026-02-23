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
#define NRF_RX_ADDR_P1  0x0B
#define NRF_DYNPD       0x1C
#define NRF_FEATURE     0x1D

// 명령 정의
#define NRF_R_RX_PAYLOAD      0x61
#define NRF_W_ACK_PAYLOAD_CMD 0xA8 // Pipe 1은 0xA9 (0xA8 | 0x01)
#define NRF_FLUSH_RX          0xE2

// 함수 선언
void nRF_Init_RX(void);                                  // 수신 모드 초기화
void nRF_ReceivePacket(uint8_t *rx_buf);                // 데이터 읽기
void nRF_LoadAckPayload(uint8_t *data, uint8_t len);    // 답장(ACK) 장전

void nRF_WriteReg(uint8_t reg, uint8_t value);
uint8_t nRF_ReadReg(uint8_t reg);
void nRF_WriteBuf(uint8_t reg, uint8_t *pBuf, uint8_t len);

#endif
