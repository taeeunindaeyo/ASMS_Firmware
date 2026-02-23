#include "main.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    SystemClock_Config();
    Delay_ms(100);
    GPIO_Init();
    USART1_Init();
    SPI1_Init();

    USART1_SendString("\r\n=== ASMS 수신기 입니당 ===\r\n");

    nRF_Init_RX();
    nRF_LoadAckPayload((uint8_t*)"RX_READY", 8);

    uint8_t rx_data[32];
    char msg[64];
    char reply[32];
    int16_t joy_x, joy_y;

    while (1)
    {
        // 데이터 수신 확인
        if (nRF_ReadReg(NRF_STATUS) & 0x40)
        {
            LED_ON();

            // 데이터 읽기
            memset(rx_data, 0, 32);
            nRF_ReceivePacket(rx_data);

            uint8_t mode = rx_data[0];

            // 모드별 처리
            if (mode == 2) // MANUAL
            {
                joy_x = (int16_t)((rx_data[3] << 8) | rx_data[2]);
                joy_y = (int16_t)((rx_data[5] << 8) | rx_data[4]);
                sprintf(msg, "[수동주행] X:%d, Y:%d\r\n", joy_x, joy_y);
                sprintf(reply, "M:OK");
            }
            else if (mode == 3) // ESTOP
            {
                sprintf(msg, "[비상정지]\r\n");
                sprintf(reply, "E:OK");
            }
            else if (mode == 1) // AUTO
            {
                sprintf(msg, "[자율주행]\r\n");
                sprintf(reply, "A:OK");
            }
            else
            {
                sprintf(msg, "[NONE]\r\n");
                sprintf(reply, "NONE");
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
