#include "main.h"
#include <stdlib.h>

#define I2C_SPEED_HZ   30000UL   // I2C 통신 속도
#define POLL_MS        10        // 메인 루프 주기
#define DEBOUNCE_N     2         // 스위치 디바운싱

#define JOY_DEADZONE 150
#define HEARTBEAT_MS 100    // 0.1초판: NONE·AUTO·ESTOP 재전송 주기 (MANUAL은 POLL_MS 그대로)

#define NRF_BOOT_TRIES   20   // 부팅 시 nRF 설정 확인 재시도 횟수 (50ms 간격, 최대 약 1.2초)
#define TX_REINIT_FAILS  100  // 송신 실패가 연속 이만큼이면 nRF 재초기화 (MANUAL 약 1초)

volatile uint8_t  dbg_nrf_init_ok = 0;  // 1 = 부팅 시 nRF 설정 확인 통과
volatile uint32_t dbg_tx_reinit   = 0;  // 동작 중 nRF 재초기화 횟수
volatile uint8_t  dbg_tx_result = 0xFF;
volatile uint32_t dbg_tx_attempts = 0;
volatile uint8_t dbg_tx_config  = 0xFF;
volatile uint8_t dbg_tx_channel = 0xFF;
volatile uint8_t dbg_tx_feature = 0xFF;

int main(void) {
    SystemClock_Config(); // 72MHz 설정
    Delay_ms(100);

    GPIO_Init();
    USART1_Init();
    SPI1_Init();

    sw_init_pullup();     // 스위치 입력 설정
    i2c1_init(I2C_SPEED_HZ, g_pclk1_hz);

    // 조이스틱 초기화
    Joystick_Init();

    // OLED 연결 확인 및 초기화
    // 전원 투입 직후엔 OLED가 늦게 깨어날 수 있어 몇 번 더 찾아본다
    uint8_t addr;
    for (int i = 0; i < 10 && oled_probe(&addr) != 0; i++) Delay_ms(100);
    while (oled_probe(&addr) != 0) {
        // 그래도 없으면 깜빡이며 계속 다시 찾는다 (리셋 없이 연결되면 바로 진행)
        blink_fast(5); Delay_ms(1000);
    }
    oled_set_addr(addr);
    oled_init();

    // nRF24L01 초기화 (설정값이 읽혀 나올 때까지 재시도)
    dbg_nrf_init_ok = nRF_Init_TX(NRF_BOOT_TRIES);

    USART1_SendString("\r\n=== ASMS(0.1s) 송신기입니당 ===\r\n");

    while(1) {
        // uint8_t manual_on = sw_raw_active_low(SW_MANUAL_PIN); // 켜져있으면 1
        JoystickNorm j = Joystick_ReadNorm();

        // 수동 스위치 켜짐 or 조이스틱이 중심에서 벗어남
        uint8_t unsafe_joy = (abs(j.x) > JOY_DEADZONE) || (abs(j.y) > JOY_DEADZONE);

        if (unsafe_joy) {
            oled_clear();
            oled_write_centered("WARNING", 10);

            // if (manual_on)  oled_write_centered("OFF MANUAL SW", 30);
            if (unsafe_joy) oled_write_centered("CENTER STICK", 45);

            // LED 깜빡임으로 경고
            LED_ON(); Delay_ms(500);
            LED_OFF(); Delay_ms(500);
        }
        else {
            // 준비 완료 메시지
            oled_clear();
            oled_write_centered("System Ready", 30);
            oled_write_centered("Start", 45);
            Delay_ms(1000);
            break;
        }
    }

    vehicle_oled_show_mode(MODE_MANUAL);

    Deb dA = {0, 0xFF, 0};
    Deb dM = {0, 0xFF, 0};
    Deb dE = {0, 0xFF, 0};

    Mode cur = MODE_NONE;
    Mode lastMode = 0xFF;

    uint16_t heartbeat_elapsed_ms = HEARTBEAT_MS;

    uint8_t tx_data[32] = {0};
    uint8_t ack_data[32] = {0};
    uint16_t tx_fail_streak = 0;

    while(1) {
        Delay_ms(POLL_MS); // 10ms 주기
        heartbeat_elapsed_ms += POLL_MS;

        // 1) 스위치 입력 읽기 및 디바운싱
        uint8_t ra = sw_raw_active_low(SW_AUTO_PIN);
        uint8_t rm = sw_raw_active_low(SW_MANUAL_PIN);
        uint8_t re = sw_raw_active_low(SW_ESTOP_PIN);

        if (ra == dA.last) {
            if (dA.cnt < DEBOUNCE_N) dA.cnt++;
            if (dA.cnt >= DEBOUNCE_N) dA.stable = ra;
        } else {
            dA.last = ra; dA.cnt = 0;
        }

        if (rm == dM.last) {
            if (dM.cnt < DEBOUNCE_N) dM.cnt++;
            if (dM.cnt >= DEBOUNCE_N) dM.stable = rm;
        } else {
            dM.last = rm; dM.cnt = 0;
        }

        if (re == dE.last) {
            if (dE.cnt < DEBOUNCE_N) dE.cnt++;
            if (dE.cnt >= DEBOUNCE_N) dE.stable = re;
            } else {
                dE.last = re; dE.cnt = 0;
            }

        // 2) 모드 결정 및 화면 갱신
        Mode next = vehicle_decide_mode(dA.stable, dM.stable, dE.stable);

        if (next != cur) {
            cur = next;
            vehicle_oled_show_mode(cur);
        }

        // 3) 조이스틱 값 읽기
        JoystickNorm joy = Joystick_ReadNorm();

        // 4) 송신 패킷 구성
        memset(tx_data, 0, 32); // 버퍼 초기화
        tx_data[0] = (uint8_t)cur; // [0]: 모드

        if (cur == MODE_MANUAL) {
            // 조이스틱 데이터 유효
            tx_data[1] = 1;

            // X축 데이터
            tx_data[2] = (uint8_t)(joy.x & 0xFF);
            tx_data[3] = (uint8_t)((joy.x >> 8) & 0xFF);

            // Y축 데이터
            tx_data[4] = (uint8_t)(joy.y & 0xFF);
            tx_data[5] = (uint8_t)((joy.y >> 8) & 0xFF);
        } else {
            tx_data[1] = 0;
        }

        // 5) 무선 전송
        // 모드 변경 or 수동 모드(조이스틱 값 연속 전송)
        // 수동 모드: 10ms마다 전송
        // 나머지 모드: 변경 즉시 전송 + 500ms마다 재전송
        if (cur == MODE_MANUAL ||
            cur != lastMode ||
            heartbeat_elapsed_ms >= HEARTBEAT_MS)
        {
            LED_ON();
            //nRF_TxPacket(tx_data, ack_data);
            dbg_tx_config  = nRF_ReadReg(NRF_CONFIG);
            dbg_tx_channel = nRF_ReadReg(NRF_RF_CH);
            dbg_tx_feature = nRF_ReadReg(NRF_FEATURE);

            // nRF 설정이 풀려 있으면 (전원 흔들림 등) 송신 전에 다시 초기화
            if (dbg_tx_config != NRF_TX_CONFIG_VAL ||
                dbg_tx_channel != NRF_CHANNEL ||
                dbg_tx_feature != 0x06)
            {
                nRF_Init_TX(1);
                dbg_tx_reinit++;
            }

            dbg_tx_attempts++;
            dbg_tx_result = nRF_TxPacket(tx_data, ack_data);

            // 1 = 송신 성공, 3 = 송신 성공 + ACK 데이터. 그 외(0 오류, 2 재전송 초과)가 계속되면 재초기화
            if (dbg_tx_result == 1 || dbg_tx_result == 3) {
                tx_fail_streak = 0;
            } else if (++tx_fail_streak >= TX_REINIT_FAILS) {
                nRF_Init_TX(1);
                dbg_tx_reinit++;
                tx_fail_streak = 0;
            }

            LED_OFF();

            heartbeat_elapsed_ms = 0;
        }

        lastMode = cur;
    }
}
