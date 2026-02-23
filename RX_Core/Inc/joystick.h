#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdint.h>

/*
 * JoystickRaw:
 *  - ADC 원시값 그대로(0~4095)
 */
typedef struct
{
    uint16_t x_raw;   // PA0
    uint16_t y_raw;   // PA1
} JoystickRaw;

/*
 * JoystickNorm:
 *  - 센터(중립) 기준으로 오프셋 형태(-2048 ~ +2047 근사)
 *  - 조이스틱 처리/데드존/스케일링에 바로 쓰기 편함
 */
typedef struct
{
    int16_t x;
    int16_t y;
} JoystickNorm;

/*
 * Joystick_Init()
 *  - 내부적으로 ADC_DMA_Init_Start() 호출
 *  - "조이스틱 입력이 계속 갱신되는 상태"를 만들어 둠
 */
void Joystick_Init(void);

/*
 * Joystick_ReadRaw()
 *  - 최신 ADC 스냅샷을 읽어 Raw(0~4095)로 반환
 */
JoystickRaw  Joystick_ReadRaw(void);

/*
 * Joystick_ReadNorm()
 *  - 최신 ADC 스냅샷을 읽어 센터 기준 오프셋(-/+) 값으로 반환
 *  - (raw - center) 형태라 데드존/명령 매핑에 유리
 */
JoystickNorm Joystick_ReadNorm(void);

#endif // JOYSTICK_H
