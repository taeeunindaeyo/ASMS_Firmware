#ifndef ADC_H
#define ADC_H

#include <stdint.h>

/*
 * ADC1 + DMA1_Channel1이 "계속 갱신"하는 Raw 버퍼
 *  - 12-bit ADC 결과(0~4095)가 DMA에 의해 자동 업데이트됨
 *  - 채널 순서: [0] = CH0(PA0), [1] = CH1(PA1)
 */
extern volatile uint16_t adc_dma_buf[2];

/*
 * ADC_DMA_Init_Start()
 *  - GPIOA(PA0, PA1) 아날로그 입력 설정
 *  - DMA1_Channel1 설정: ADC1->DR -> adc_dma_buf[2]로 원형(CIRC) 전송
 *  - ADC1 설정: Scan + Continuous + DMA 모드
 *  - 캘리브레이션 후 SWSTART로 변환 시작
 */
void ADC_DMA_Init_Start(void);

/*
 * ADC_DMA_CopyLatest(out)
 *  - DMA가 갱신 중인 2채널 값을 "한 번에" 스냅샷으로 안전하게 복사
 *  - out[0]=PA0, out[1]=PA1
 *  - 간단한 임계구역(인터럽트 off)로 2개 값이 섞여 읽히는 것을 방지
 */
void ADC_DMA_CopyLatest(uint16_t out[2]);

#endif // ADC_H
