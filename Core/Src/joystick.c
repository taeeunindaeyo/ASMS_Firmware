#include "joystick.h"
#include "adc.h"

#define JOY_CENTER 2048  // 12-bit ADC 중립값(대략)
#define JOY_DEADZONE 150 

// 조이스틱 입력 시작(= ADC DMA 변환 스타트)
void Joystick_Init(void)
{
    ADC_DMA_Init_Start();
}

// Raw(0~4095)로 읽기
JoystickRaw Joystick_ReadRaw(void)
{
    uint16_t v[2];
    ADC_DMA_CopyLatest(v);

    JoystickRaw r;
    r.x_raw = v[0];
    r.y_raw = v[1];
    return r;
}

// Center 기준 오프셋(-/+ )으로 읽기
JoystickNorm Joystick_ReadNorm(void)
{
    uint16_t v[2];
    ADC_DMA_CopyLatest(v);

    JoystickNorm n;
    n.x = (int16_t)v[0] - JOY_CENTER;
    n.y = (int16_t)v[1] - JOY_CENTER;

    // |150|보다 작으면 0으로 처리
    if (raw_x > -JOY_DEADZONE && raw_x < JOY_DEADZONE) n.x = 0;
    else n.x = raw_x;

    if (raw_y > -JOY_DEADZONE && raw_y < JOY_DEADZONE) n.y = 0;
    else n.y = raw_y;
    
    return n;
}

