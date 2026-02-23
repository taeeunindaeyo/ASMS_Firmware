#include "main.h"

#define I2C_SPEED_HZ   50000UL   // 너 환경에서 안정적이던 값
#define POLL_MS        10
#define DEBOUNCE_N     2         // 10ms*2=20ms 정도로 빠르게

static uint32_t g_pclk1_hz  = 36000000UL; // 36MHz

int main(void)
{
		SystemClock_Config();
		Delay_ms(100);
		GPIO_Init();
		USART1_Init();
		SPI1_Init();

		sw_init_pullup();
		i2c1_init(I2C_SPEED_HZ, g_pclk1_hz);

		uint8_t addr;
    if (oled_probe(&addr) != 0) {
        while(1) { blink_fast(5); Delay_ms(1000); }
    }

    oled_set_addr(addr);
    oled_init();

    USART1_SendString("[OK] OLED 연결 성공!\r\n");

    nRF_Init_TX();
	USART1_SendString("\r\n=== ACK 있는 송신기 입니당 ===\r\n");

	vehicle_oled_show_mode(MODE_NONE);

    Deb dA = {0, 0xFF, 0};
    Deb dM = {0, 0xFF, 0};
    Deb dE = {0, 0xFF, 0};

    Mode cur = MODE_NONE;

	uint8_t tx_data[32] = " 송신 성공! ";
	uint8_t ack_data[32];

	while(1) {
		Delay_ms(POLL_MS); // 10ms

		uint8_t ra = sw_raw_active_low(SW_AUTO_PIN);
		uint8_t rm = sw_raw_active_low(SW_MANUAL_PIN);
		uint8_t re = sw_raw_active_low(SW_ESTOP_PIN);

		// debounce (DEBOUNCE_N번 연속 같으면 stable 갱신)
		if (ra == dA.last) { if (dA.cnt < DEBOUNCE_N) dA.cnt++; if (dA.cnt >= DEBOUNCE_N) dA.stable = ra; }
		else { dA.last = ra; dA.cnt = 0; }

		if (rm == dM.last) { if (dM.cnt < DEBOUNCE_N) dM.cnt++; if (dM.cnt >= DEBOUNCE_N) dM.stable = rm; }
		else { dM.last = rm; dM.cnt = 0; }

		if (re == dE.last) { if (dE.cnt < DEBOUNCE_N) dE.cnt++; if (dE.cnt >= DEBOUNCE_N) dE.stable = re; }
		else { dE.last = re; dE.cnt = 0; }

		Mode next = vehicle_decide_mode(dA.stable, dM.stable, dE.stable);

		if (next != cur) {
		   // 1. 화면 갱신 (팀원 코드)
			cur = next;
			vehicle_oled_show_mode(cur);

			// 2. [중요] 무선 전송 코드 (이게 꼭 있어야 수신부로 신호가 갑니다!)
			memset(tx_data, 0, 32);
			tx_data[0] = (uint8_t)cur;

			LED_ON();
			uint8_t res = nRF_TxPacket(tx_data, ack_data);
			LED_OFF();

			// 디버깅 메시지
			 char msg[64];
			 if (res == 1) {
				 sprintf(msg, "송신 성공 (Mode: %d) -> No Data\r\n", cur);
		         USART1_SendString(msg);
		     } else if (res == 3) { // ACK Payload 수신
		         sprintf(msg, "송신 성공 (Mode: %d) & 수신 답장: \r\n", cur);
		         USART1_SendString((char*)ack_data);
		         USART1_SendString("\r\n");
		     } else if (res == 2) {
		         USART1_SendString("송신 실패: No ACK\r\n");
		     } else {
		         USART1_SendString("송신 실패: Error\r\n");
		     }
		}
	}
}
