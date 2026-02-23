#include "adc.h"
#include "stm32f1xx.h"

volatile uint16_t adc_dma_buf[2];  // DMA가 계속 덮어쓰는 최신 ADC 결과 버퍼

/*
 * PA0/PA1을 ADC 입력(Analog)으로 설정
 */
static void adc_gpio_init(void)
{
    // GPIOA 클럭 인가
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    // PA0, PA1: Analog input (CNF=00, MODE=00)
    // CRL: 핀 0~7 설정 (각 핀 4비트)
    GPIOA->CRL &= ~((0xFU << (0 * 4)) | (0xFU << (1 * 4)));
}

/*
 * DMA1_Channel1 설정
 *  - Peripheral: ADC1->DR
 *  - Memory    : adc_dma_buf
 *  - Length    : 2 (채널 2개)
 *  - Circular  : 계속 반복해서 최신 값 유지
 */
static void adc_dma_init(void)
{
    // DMA1 클럭 인가
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;

    // 설정 전 채널 disable
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;

    DMA1_Channel1->CPAR  = (uint32_t)&ADC1->DR;       // 읽을 곳(ADC Data Register)
    DMA1_Channel1->CMAR  = (uint32_t)adc_dma_buf;     // 쓸 곳(메모리 버퍼)
    DMA1_Channel1->CNDTR = 2;                         // 전송 개수(2개)

    // Circular + MINC + 16bit/16bit + P->M + medium priority
    DMA1_Channel1->CCR =
        DMA_CCR_CIRC |         // 원형 버퍼
        DMA_CCR_MINC |         // 메모리 주소 증가
        DMA_CCR_PSIZE_0 |      // Peripheral size 16-bit
        DMA_CCR_MSIZE_0 |      // Memory size 16-bit
        DMA_CCR_PL_0;          // Priority medium

    // DMA enable
    DMA1_Channel1->CCR |= DMA_CCR_EN;
}

/*
 * ADC1 설정 및 변환 시작
 *  - Scan(2채널) + Continuous + DMA
 *  - Calibration 포함
 */
static void adc_core_init(void)
{
    // ADC prescaler: PCLK2/6 (72MHz -> 12MHz)
    RCC->CFGR &= ~RCC_CFGR_ADCPRE;
    RCC->CFGR |=  RCC_CFGR_ADCPRE_DIV6;

    // ADC1 클럭 인가
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    // Scan mode(정규 시퀀스 여러 채널), Continuous, DMA
    ADC1->CR1 = ADC_CR1_SCAN;
    ADC1->CR2 = ADC_CR2_CONT | ADC_CR2_DMA;

    // Sample time CH0, CH1: 55.5 cycles (노이즈/임피던스 고려해서 여유있게)
    ADC1->SMPR2 &= ~((0x7U << (0 * 3)) | (0x7U << (1 * 3)));
    ADC1->SMPR2 |=  ((0x5U << (0 * 3)) | (0x5U << (1 * 3)));

    // Regular sequence length = 2 conversions
    // L = (개수-1), 2개면 L=1
    ADC1->SQR1 &= ~ADC_SQR1_L;
    ADC1->SQR1 |= (1U << 20);

    // 1st=CH0, 2nd=CH1
    ADC1->SQR3 = (0U << 0) | (1U << 5);

    // ADC ON
    ADC1->CR2 |= ADC_CR2_ADON;
    for (volatile int i = 0; i < 1000; i++) { __NOP(); }

    // Reset calibration
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    while (ADC1->CR2 & ADC_CR2_RSTCAL) {}

    // Calibration
    ADC1->CR2 |= ADC_CR2_CAL;
    while (ADC1->CR2 & ADC_CR2_CAL) {}

    // Start conversion (관례상 ADON 재세트 후 SWSTART)
    ADC1->CR2 |= ADC_CR2_ADON;
    ADC1->CR2 |= ADC_CR2_SWSTART;
}

/*
 * 외부에서 호출하는 "원샷 스타트 함수"
 */
void ADC_DMA_Init_Start(void)
{
    adc_gpio_init();
    adc_dma_init();
    adc_core_init();
}

/*
 * DMA 갱신값 2개를 한 번에 스냅샷
 *  - 2개 값이 서로 다른 시점으로 섞여 읽히는 문제 방지
 */
void ADC_DMA_CopyLatest(uint16_t out[2])
{
    __disable_irq();
    out[0] = adc_dma_buf[0];
    out[1] = adc_dma_buf[1];
    __enable_irq();
}
