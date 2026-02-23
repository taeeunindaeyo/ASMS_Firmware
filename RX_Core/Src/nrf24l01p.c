#include "nrf24l01p.h"
#include "gpio.h"
#include "spi.h"
#include "delay.h"

static uint8_t ADDR[5] = {0xB0, 0xB1, 0xB2, 0xB3, 0xB4};

void nRF_Init_TX(void) {
		CE_LOW();
		nRF_WriteReg(NRF_CONFIG, 0x0A); // TX mode
		Delay_ms(2);
		
		nRF_WriteReg(NRF_EN_AA, 0x01);
		nRF_WriteReg(NRF_EN_RXADDR, 0x01);
		nRF_WriteReg(NRF_SETUP_AW, 0x03);
		nRF_WriteReg(NRF_SETUP_RETR, 0x2F);
		nRF_WriteReg(NRF_RF_CH, 76);
		nRF_WriteReg(NRF_RF_SETUP, 0x0E);
		
		nRF_WriteReg(NRF_FEATURE, 0x06);
		nRF_WriteReg(NRF_DYNPD, 0x03);
		
		nRF_WriteBuf(NRF_TX_ADDR, ADDR, 5);
		nRF_WriteBuf(NRF_RX_ADDR_P0, ADDR, 5);
}

uint8_t nRF_TxPacket(uint8_t *tx_buf, uint8_t *ack_buf) {
		uint8_t status;
		uint32_t timeout = 0xFFFFF;
		
		nRF_WriteReg(NRF_FLUSH_TX, 0xFF);
		
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
		
		if(status & 0x20) { // TX_DS
				if(status & 0x40) { // RX_DR
						CSN_LOW();
						SPI1_Transfer(NRF_R_RX_PAYLOAD);
						for(int i=0; i<32; i++) ack_buf[i] = SPI1_Transfer(0xFF);
						CSN_HIGH();
						nRF_WriteReg(NRF_STATUS, 0x40);
						return 3;
				}
				return 1;
		}
		
		if(status & 0x10) { // MAX_RT
				nRF_WriteReg(NRF_FLUSH_TX, 0xFF);
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
			