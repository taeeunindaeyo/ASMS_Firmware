#include "delay.h"

void Delay_ms(uint32_t ms) {
		for(volatile int i=0; i<ms*7200; i++);
}

void Delay_us(uint32_t us) {
		for(volatile int i=0; i<us*72; i++);
}