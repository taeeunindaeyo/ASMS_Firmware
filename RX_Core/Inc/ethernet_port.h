#ifndef __Ethernet_PORT_H
#define __Ethernet_PORT_H

#include "stm32f1xx.h"
#include <stdint.h>

// 반환: 0 = 정상, -1 = W5500 응답 없음
int8_t Ethernet_Init(uint32_t timeout_ms);

#endif
