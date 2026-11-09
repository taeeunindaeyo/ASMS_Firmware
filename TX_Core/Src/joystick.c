// 자동초점코드

#include "joystick.h"
#include "adc.h"

extern void Delay_ms(uint32_t ms);

static uint16_t joy_center_x = 2048;
static uint16_t joy_center_y = 2048;

// 추가
volatile uint16_t dbg_adc_raw_x = 0;
volatile uint16_t dbg_adc_raw_y = 0;

volatile uint16_t dbg_joy_center_x = 2048;
volatile uint16_t dbg_joy_center_y = 2048;

volatile int16_t dbg_joy_offset_x = 0;
volatile int16_t dbg_joy_offset_y = 0;

volatile int16_t dbg_joy_norm_x = 0;
volatile int16_t dbg_joy_norm_y = 0;
//

#define CALIBRATION_SAMPLES 32
#define JOY_DEADZONE 150

// 축 교환: 논리 X = ADC[1](PA1), 논리 Y = ADC[0](PA0)
// 아래 비대칭 보정값은 PA1 물리 축을 측정한 값이므로 논리 X에 적용한다.
#define JOY_X_NEG_MAX   2008
#define JOY_X_POS_MAX   2079
#define JOY_X_OUT_MAX   2043

// 조이스틱 입력 시작(= ADC DMA 변환 스타트)
void Joystick_Init(void)
{
    // 1. ADC 및 DMA 시작
    ADC_DMA_Init_Start();

    // 2. DMA가 첫 데이터들을 채우고 안정화될 때까지 대기
    Delay_ms(10);

    uint32_t sum_x = 0;
    uint32_t sum_y = 0;
    uint16_t v[2];

    // 3. 스틱이 중립인 상태에서 여러 번 샘플링하여 누적
    for(int i = 0; i < CALIBRATION_SAMPLES; i++) {
        ADC_DMA_CopyLatest(v);
        sum_x += v[1];   // 논리 X = PA1
        sum_y += v[0];   // 논리 Y = PA0

        // 노이즈 분산을 위해 샘플링 사이 일정한 딜레이
        Delay_ms(2);
    }

    // 4. 평균값을 구해 동적 영점으로 설정
    joy_center_x = sum_x / CALIBRATION_SAMPLES;
    joy_center_y = sum_y / CALIBRATION_SAMPLES;

    dbg_joy_center_x = joy_center_x;  // 추가
    dbg_joy_center_y = joy_center_y;  // 추가
}

// Raw(0~4095)로 읽기
JoystickRaw Joystick_ReadRaw(void)
{
    uint16_t v[2];
    ADC_DMA_CopyLatest(v);

    dbg_adc_raw_x = v[1]; // 추가 (논리 X = PA1)
    dbg_adc_raw_y = v[0]; // 추가 (논리 Y = PA0)

    JoystickRaw r;
    r.x_raw = v[1];
    r.y_raw = v[0];
    return r;
}

// x축 스케일 (+,- 비율이 달라서..)  ※ 축 교환 전 "Y축 스케일"이던 함수
static int16_t Joystick_ScaleX(int16_t x)
{
    int32_t out;

    if (x > 0)
    {
        out = (int32_t)x * JOY_X_OUT_MAX / JOY_X_POS_MAX;

        if (out > JOY_X_OUT_MAX)
        {
            out = JOY_X_OUT_MAX;
        }
    }
    else if (x < 0)
    {
        out = (int32_t)x * JOY_X_OUT_MAX / JOY_X_NEG_MAX;

        if (out < -JOY_X_OUT_MAX)
        {
            out = -JOY_X_OUT_MAX;
        }
    }
    else
    {
        out = 0;
    }

    return (int16_t)out;
}

// Center 기준 오프셋(-/+ )으로 읽기
JoystickNorm Joystick_ReadNorm(void)
{
    uint16_t v[2];
    ADC_DMA_CopyLatest(v);

    dbg_adc_raw_x = v[1]; // 추가 (논리 X = PA1)
    dbg_adc_raw_y = v[0]; // 추가 (논리 Y = PA0)

    JoystickNorm n;

    int16_t raw_x = (int16_t)v[1] - joy_center_x;   // 논리 X = PA1
    int16_t raw_y = (int16_t)v[0] - joy_center_y;   // 논리 Y = PA0

    dbg_joy_offset_x = raw_x; // 추가
    dbg_joy_offset_y = raw_y; // 추가

    // |150|보다 작으면 0으로 처리
    if (raw_x > -JOY_DEADZONE && raw_x < JOY_DEADZONE) n.x = 0;
    else n.x = Joystick_ScaleX(raw_x);

    if (raw_y > -JOY_DEADZONE && raw_y < JOY_DEADZONE) n.y = 0;
    else n.y = raw_y;

    dbg_joy_norm_x = n.x; // 추가
    dbg_joy_norm_y = n.y;

    return n;
}
