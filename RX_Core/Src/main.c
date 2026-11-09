#include "main.h"
#include <stdio.h>
#include <string.h>

#include "wizchip_conf.h"
#include "socket.h"
#include "Ethernet_port.h"

#define UDP_SOCKET_NUM 0

wiz_NetInfo gWIZNETINFO = {
    .mac  = {0x00, 0x08, 0xDC, 0x11, 0x22, 0x33}, // 임의의 MAC 주소
    .ip   = {10, 177, 21, 5},
    .sn   = {255, 255, 255, 0},
    .gw   = {10, 177, 21, 254},
    .dns  = {8, 8, 8, 8},
    .dhcp = NETINFO_STATIC
};

// 개별 IP 대신 브로드캐스트 IP 주소 하나만 사용 (끝자리 255)
uint8_t dest_ip_broadcast[4] = {10, 177, 21, 255};
uint16_t dest_port = 5000;

// 디버깅용 전역변수
volatile uint32_t dbg_rx_count = 0;
volatile uint8_t  dbg_rx_mode  = 0xFF;
volatile int16_t  dbg_rx_x     = 0;
volatile int16_t  dbg_rx_y     = 0;

volatile uint8_t dbg_nrf_config  = 0xFF;
volatile uint8_t dbg_nrf_channel = 0xFF;
volatile uint8_t dbg_nrf_feature = 0xFF;
volatile uint8_t dbg_nrf_status  = 0xFF;
volatile uint8_t dbg_nrf_fifo    = 0xFF;

#define NRF_BOOT_TRIES       20    // 부팅 시 nRF 설정 확인 재시도 횟수 (50ms 간격)
#define ETH_BOOT_WAIT_MS     1500  // 부팅 시 W5500 응답을 기다리는 최대 시간
#define ETH_RETRY_MS         1000  // W5500이 없으면 이 간격으로 다시 시도
#define RX_IDLE_REINIT_MS    3000  // 이 시간 동안 수신이 없으면 nRF 재초기화

volatile uint8_t  dbg_nrf_init_ok = 0;  // 1 = 부팅 시 nRF 설정 확인 통과
volatile uint8_t  dbg_eth_ok      = 0;  // 1 = W5500 초기화·소켓 열기 완료
volatile uint32_t dbg_rx_reinit   = 0;  // 동작 중 nRF 재초기화 횟수

// DWT 사이클 카운터로 경과 ms 계산 (루프가 59초 안에 한 번 이상 부르면 됨)
static uint32_t now_ms(void)
{
    static uint32_t last_cyc = 0, rem = 0, ms = 0;
    uint32_t cyc = DWT->CYCCNT;
    uint32_t cpm = g_sysclk_hz / 1000U;

    rem += cyc - last_cyc;
    last_cyc = cyc;
    ms  += rem / cpm;
    rem %= cpm;
    return ms;
}

// W5500 초기화 + 네트워크 설정 + UDP 소켓 열기. 응답 없으면 소켓은 열지 않는다
static uint8_t Ethernet_Start(uint32_t wait_ms)
{
    if (Ethernet_Init(wait_ms) != 0) return 0;
    ctlnetwork(CN_SET_NETINFO, (void*)&gWIZNETINFO); // IP, MAC 적용
    if (socket(UDP_SOCKET_NUM, Sn_MR_UDP, 5000, 0) != UDP_SOCKET_NUM) return 0;
    return 1;
}

// nRF 재초기화 후 첫 답장 다시 장전
static void nRF_Restart(void)
{
    nRF_Init_RX(1);
    nRF_LoadAckPayload((uint8_t*)"RX_READY", 8);
    dbg_rx_reinit++;
}

int main(void)
{
    SystemClock_Config();
    Delay_ms(100);
    GPIO_Init();

    blink_fast(3);    // 진단: MCU가 여기까지 실행되면 3번 점멸
    Delay_ms(1000);

    USART1_Init();
    SPI1_Init();
    towerlamp_init();

    USART1_SendString("\r\n=== ASMS 수신기 입니당 ===\r\n");

    // nRF 초기화를 먼저 한다 (이더넷 대기 때문에 무선 시작이 막히지 않게)
    dbg_nrf_init_ok = nRF_Init_RX(NRF_BOOT_TRIES);
    nRF_LoadAckPayload((uint8_t*)"RX_READY", 8);
    USART1_SendString(dbg_nrf_init_ok ? " nRF 초기화 완료\r\n" : " nRF 설정 확인 실패 (동작 중 재시도)\r\n");

    // --- W5500 초기화 및 네트워크 설정 (응답 없으면 건너뛰고 나중에 재시도) ---
    dbg_eth_ok = Ethernet_Start(ETH_BOOT_WAIT_MS);
    USART1_SendString(dbg_eth_ok ? " 이더넷 초기화, 소켓 오픈 완료\r\n" : " W5500 응답 없음 (1초마다 재시도)\r\n");

    blink_fast(2);    // 진단: 초기화 함수까지 끝나면 2번 점멸

    uint8_t rx_data[32];
    char msg[64];
    char reply[32];
    int16_t joy_x, joy_y;

    uint32_t last_rx_ms  = now_ms();
    uint32_t last_eth_ms = last_rx_ms;

    while (1)
    {
    	uint32_t t = now_ms();

    	// 진단용: nRF 설정과 상태 읽기
    	dbg_nrf_config  = nRF_ReadReg(NRF_CONFIG);
    	dbg_nrf_channel = nRF_ReadReg(NRF_RF_CH);
    	dbg_nrf_feature = nRF_ReadReg(NRF_FEATURE);
    	dbg_nrf_fifo    = nRF_ReadReg(NRF_FIFO_STATUS);
    	dbg_nrf_status  = nRF_ReadReg(NRF_STATUS);

    	// nRF 설정이 풀렸거나(전원 흔들림 등) 한동안 아무것도 못 받았으면 다시 초기화
    	if (dbg_nrf_config != NRF_RX_CONFIG_VAL ||
    	    dbg_nrf_channel != NRF_CHANNEL ||
    	    dbg_nrf_feature != 0x06 ||
    	    (uint32_t)(t - last_rx_ms) >= RX_IDLE_REINIT_MS)
    	{
    	    nRF_Restart();
    	    last_rx_ms = t;
    	    continue;
    	}

    	// W5500이 없었으면 주기적으로 다시 시도 (짧게 기다려 무선 처리를 막지 않는다)
    	if (!dbg_eth_ok && (uint32_t)(t - last_eth_ms) >= ETH_RETRY_MS)
    	{
    	    last_eth_ms = t;
    	    dbg_eth_ok = Ethernet_Start(5);
    	}

    	if ((dbg_nrf_fifo & 0x01) == 0)  // RX_EMPTY=0: FIFO에 읽을 패킷 있음
    	{
            LED_ON();

            memset(rx_data, 0, 32);
            if (nRF_ReceivePacket(rx_data) == 0)  // 잘못된 길이 → 버리고 다음 루프
            {
                LED_OFF();
                continue;
            }

            uint8_t mode = rx_data[0];

            joy_x = (int16_t)((rx_data[3] << 8) | rx_data[2]);
            joy_y = (int16_t)((rx_data[5] << 8) | rx_data[4]);


            // 여기 추가
            dbg_rx_mode = mode;
            dbg_rx_x = joy_x;
            dbg_rx_y = joy_y;
            dbg_rx_count++;
            last_rx_ms = t;


            // 5바이트 UDP 패킷
            uint8_t udp_packet[5];
            udp_packet[0] = mode;
            udp_packet[1] = (uint8_t)(joy_x & 0xFF);        // X축 하위 8비트
            udp_packet[2] = (uint8_t)((joy_x >> 8) & 0xFF); // X축 상위 8비트
            udp_packet[3] = (uint8_t)(joy_y & 0xFF);        // Y축 하위 8비트
            udp_packet[4] = (uint8_t)((joy_y >> 8) & 0xFF); // Y축 상위 8비트

            // 브로드캐스트 전송 (W5500이 없으면 건너뜀: 없는 칩에 보내면 무한 대기)
            if (dbg_eth_ok)
            {
                sendto(UDP_SOCKET_NUM, udp_packet, 5, dest_ip_broadcast, dest_port);
            }

            towerlamp_all_off();

            if (mode == 2) // MANUAL
            {
                joy_x = (int16_t)((rx_data[3] << 8) | rx_data[2]);
                joy_y = (int16_t)((rx_data[5] << 8) | rx_data[4]);
                sprintf(msg, "[수동주행] X:%d, Y:%d\r\n", joy_x, joy_y);
                sprintf(reply, "M:OK");
                YLED_ON();
            }
            else if (mode == 3) // ESTOP
            {
                sprintf(msg, "[비상정지]\r\n");
                sprintf(reply, "E:OK");
                RLED_ON();
            }
            else if (mode == 1) // AUTO
            {
                sprintf(msg, "[자율주행]\r\n");
                sprintf(reply, "A:OK");
                BLED_ON();
            }
            else
            {
                sprintf(msg, "[NONE]\r\n");
                sprintf(reply, "NONE");
                towerlamp_all_off();
            }


            // 결과 출력 및 답장 장전
            USART1_SendString(msg);

            // FIFO를 다 비웠을 때만 답장 장전 (ACK FIFO 최대 3개 초과 방지)
            if (nRF_ReadReg(NRF_FIFO_STATUS) & 0x01)
            {
                nRF_LoadAckPayload((uint8_t*)reply, strlen(reply));
            }

            LED_OFF();
        }
    }
}
