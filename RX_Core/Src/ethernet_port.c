#include "ethernet_port.h"
#include "gpio.h"
#include "spi.h"
#include "wizchip_conf.h"
#include "w5500.h"
#include "delay.h"

// 1. CS 핀 제어 함수 연결
static void W5500_Select(void) {
    CS_LOW();
}

static void W5500_Unselect(void) {
    CS_HIGH();
}

// 2. SPI 읽기/쓰기 함수 연결
static void W5500_Write(uint8_t byte) {
    SPI1_Transfer(byte);
}

static uint8_t W5500_Read(void) {
    return SPI1_Transfer(0xFF);
}

// W5500이 SPI에 응답할 때까지 기다림 (VERSIONR은 항상 0x04)
static int8_t W5500_WaitReady(uint32_t timeout_ms) {
    while (1) {
        if (getVERSIONR() == 0x04) return 0;
        if (timeout_ms == 0) return -1;
        Delay_ms(1);
        timeout_ms--;
    }
}

// 3. W5500 라이브러리 초기화 및 등록
// 반환: 0 = 정상, -1 = W5500 응답 없음 (이 상태로 socket()을 부르면 무한 대기하므로 호출하지 말 것)
int8_t Ethernet_Init(uint32_t timeout_ms) {
    // 라이브러리에 내가 만든 함수들을 등록
    reg_wizchip_cs_cbfunc(W5500_Select, W5500_Unselect);
    reg_wizchip_spi_cbfunc(W5500_Read, W5500_Write);

    // 전원 투입 직후엔 W5500이 아직 준비 안 됐을 수 있음
    if (W5500_WaitReady(timeout_ms) != 0) return -1;

    // W5500 내부 메모리(TX/RX 버퍼)를 각 소켓당 2KB씩 할당
    uint8_t memsize[2][8] = {{2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2}};
    if(ctlwizchip(CW_INIT_WIZCHIP, (void*)memsize) == -1) return -1;

    // 내부 소프트 리셋 뒤 다시 응답하는지 확인
    if (W5500_WaitReady(100) != 0) return -1;
    return 0;
}
