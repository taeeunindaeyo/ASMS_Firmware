#include "nrf24l01p.h"
#include "gpio.h"
#include "spi.h"
#include "delay.h"

static uint8_t ADDR[5] = {0xB0, 0xB1, 0xB2, 0xB3, 0xB4};

// 레지스터 한 번 설정 (전원 다운 → FIFO·IRQ 정리 → 설정 → 전원 업)
static void nRF_Configure_TX(void) {
		CE_LOW();
		nRF_WriteReg(NRF_CONFIG, 0x08); // Power Down (EN_CRC만 유지)
		Delay_ms(2);

		nRF_Command(NRF_FLUSH_TX);      // 이전 FIFO 정리 (MCU만 리셋되면 남아 있을 수 있음)
		nRF_Command(NRF_FLUSH_RX);
		nRF_WriteReg(NRF_STATUS, 0x70); // RX_DR, TX_DS, MAX_RT 클리어

		nRF_WriteReg(NRF_EN_AA, 0x01);
		nRF_WriteReg(NRF_EN_RXADDR, 0x01);
		nRF_WriteReg(NRF_SETUP_AW, 0x03);
		nRF_WriteReg(NRF_SETUP_RETR, 0x2F);
		nRF_WriteReg(NRF_RF_CH, NRF_CHANNEL);
		nRF_WriteReg(NRF_RF_SETUP, 0x0E);

		nRF_WriteReg(NRF_FEATURE, 0x06);
		if (nRF_ReadReg(NRF_FEATURE) != 0x06) {
				// 일부 nRF24L01(구형/호환칩)은 ACTIVATE 후에만 FEATURE가 써진다
				CSN_LOW(); SPI1_Transfer(NRF_ACTIVATE); SPI1_Transfer(0x73); CSN_HIGH();
				nRF_WriteReg(NRF_FEATURE, 0x06);
		}
		nRF_WriteReg(NRF_DYNPD, 0x03);

		nRF_WriteBuf(NRF_TX_ADDR, ADDR, 5);
		nRF_WriteBuf(NRF_RX_ADDR_P0, ADDR, 5);

		nRF_WriteReg(NRF_CONFIG, NRF_TX_CONFIG_VAL); // Power Up, TX mode
		Delay_ms(5);                                 // Power Down → Standby 안정화 (1.5ms 이상)
}

// 써 놓은 값이 그대로 읽히는지 확인
uint8_t nRF_CheckConfig_TX(void) {
		uint8_t a[5];

		if (nRF_ReadReg(NRF_CONFIG)     != NRF_TX_CONFIG_VAL) return 0;
		if (nRF_ReadReg(NRF_EN_AA)      != 0x01) return 0;
		if (nRF_ReadReg(NRF_SETUP_RETR) != 0x2F) return 0;
		if (nRF_ReadReg(NRF_RF_CH)      != NRF_CHANNEL) return 0;
		if (nRF_ReadReg(NRF_RF_SETUP)   != 0x0E) return 0;
		if (nRF_ReadReg(NRF_FEATURE)    != 0x06) return 0;
		if (nRF_ReadReg(NRF_DYNPD)      != 0x03) return 0;

		nRF_ReadBuf(NRF_TX_ADDR, a, 5);
		for (int i = 0; i < 5; i++) if (a[i] != ADDR[i]) return 0;
		nRF_ReadBuf(NRF_RX_ADDR_P0, a, 5);
		for (int i = 0; i < 5; i++) if (a[i] != ADDR[i]) return 0;

		return 1;
}

// 설정 → 확인을 tries번까지 반복. 전원 투입 직후 nRF가 아직 준비 안 됐을 때를 대비
uint8_t nRF_Init_TX(uint8_t tries) {
		for (uint8_t t = 0; t < tries; t++) {
				nRF_Configure_TX();
				if (nRF_CheckConfig_TX()) return 1;
				Delay_ms(50);
		}
		return 0;
}

uint8_t nRF_TxPacket(uint8_t *tx_buf, uint8_t *ack_buf) {
		uint8_t status;
		uint32_t timeout = 0xFFFFF;

		nRF_Command(NRF_FLUSH_TX);

		CE_LOW();
		nRF_WriteBuf(NRF_W_TX_PAYLOAD, tx_buf, 32);
		CE_HIGH();
		Delay_us(20);
		CE_LOW();

		do {
				status = nRF_ReadReg(NRF_STATUS);
				timeout--;
		} while (!(status & 0x30) && timeout > 0);

		nRF_WriteReg(NRF_STATUS, 0x30);

		if (timeout == 0) return 0; // Error
		if (status == 0xFF) return 0; // SPI 응답 없음 (모든 비트 1로 읽힘) → 성공으로 오인 방지

		if(status & 0x20) { // TX_DS
				if(status & 0x40) { // RX_DR
						uint8_t len;

						// ACK 데이터 실제 길이 확인
						CSN_LOW();
						SPI1_Transfer(NRF_R_RX_PL_WID);
						len = SPI1_Transfer(0xFF);
						CSN_HIGH();

						if (len == 0 || len > 32) { // 잘못된 길이: 비우고 ACK 데이터 무시
								CSN_LOW();
								SPI1_Transfer(NRF_FLUSH_RX);
								CSN_HIGH();
								nRF_WriteReg(NRF_STATUS, 0x40);
								return 1;
						}

						CSN_LOW();
						SPI1_Transfer(NRF_R_RX_PAYLOAD);
						for(int i=0; i<len; i++) ack_buf[i] = SPI1_Transfer(0xFF);
						CSN_HIGH();
						for(int i=len; i<32; i++) ack_buf[i] = 0; // 이전 ACK 잔여 바이트 제거
						nRF_WriteReg(NRF_STATUS, 0x40);
						return 3;
				}
				return 1;
		}

		if(status & 0x10) { // MAX_RT
				nRF_Command(NRF_FLUSH_TX);
				return 2;
		}
		return 0;
}

void nRF_WriteReg(uint8_t reg, uint8_t value) {
		CSN_LOW();
		SPI1_Transfer(reg | 0x20);
		SPI1_Transfer(value);
		CSN_HIGH();
}

uint8_t nRF_ReadReg(uint8_t reg) {
		uint8_t val;
		CSN_LOW();
		SPI1_Transfer(reg & 0x1F);
		val = SPI1_Transfer(0xFF);
		CSN_HIGH();
		return val;
}

void nRF_WriteBuf(uint8_t reg, uint8_t *b, uint8_t l) {
		CSN_LOW();
		SPI1_Transfer(reg | 0x20);
		for(int i=0; i<l; i++) SPI1_Transfer(b[i]);
		CSN_HIGH();
}

void nRF_ReadBuf(uint8_t reg, uint8_t *b, uint8_t l) {
		CSN_LOW();
		SPI1_Transfer(reg & 0x1F);
		for(int i=0; i<l; i++) b[i] = SPI1_Transfer(0xFF);
		CSN_HIGH();
}

// 데이터 바이트 없는 1바이트 명령 (FLUSH_TX, FLUSH_RX 등)
void nRF_Command(uint8_t cmd) {
		CSN_LOW();
		SPI1_Transfer(cmd);
		CSN_HIGH();
}
