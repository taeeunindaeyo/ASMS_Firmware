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

int main(void)
{
    SystemClock_Config();
    Delay_ms(100);
    GPIO_Init();
    USART1_Init();
    SPI1_Init();
    void towerlamp_init();

    USART1_SendString("\r\n=== ASMS 수신기 입니당 ===\r\n");

    // --- W5500 초기화 및 네트워크 설정 ---
    Ethernet_Init();
    ctlnetwork(CN_SET_NETINFO, (void*)&gWIZNETINFO); // IP, MAC 적용
    socket(UDP_SOCKET_NUM, Sn_MR_UDP, 5000, 0);      // 포트 5000으로 UDP 소켓 열기 (수신 포트 일치)
    USART1_SendString(" 이더넷 초기화, 소켓 오픈 완료\r\n");

    // nRF 초기화
    nRF_Init_RX();
    nRF_LoadAckPayload((uint8_t*)"RX_READY", 8);

    uint8_t rx_data[32];
    char msg[64];
    char reply[32];
    int16_t joy_x, joy_y;

    while (1)
    {
        if (nRF_ReadReg(NRF_STATUS) & 0x40)
        {
            LED_ON();

            memset(rx_data, 0, 32);
            nRF_ReceivePacket(rx_data);

            uint8_t mode = rx_data[0];

            joy_x = (int16_t)((rx_data[3] << 8) | rx_data[2]);
            joy_y = (int16_t)((rx_data[5] << 8) | rx_data[4]);

            // 5바이트 UDP 패킷
            uint8_t udp_packet[5];
            udp_packet[0] = mode;
            udp_packet[1] = (uint8_t)(joy_x & 0xFF);        // X축 하위 8비트
            udp_packet[2] = (uint8_t)((joy_x >> 8) & 0xFF); // X축 상위 8비트
            udp_packet[3] = (uint8_t)(joy_y & 0xFF);        // Y축 하위 8비트
            udp_packet[4] = (uint8_t)((joy_y >> 8) & 0xFF); // Y축 상위 8비트

            // 브로드캐스트 전송
            sendto(UDP_SOCKET_NUM, udp_packet, 5, dest_ip_broadcast, dest_port);

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
            nRF_LoadAckPayload((uint8_t*)reply, strlen(reply));

            // 인터럽트 클리어
            nRF_WriteReg(NRF_STATUS, 0x40);
            LED_OFF();
        }
    }
}
