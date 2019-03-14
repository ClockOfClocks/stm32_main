#include "stm32f103xb.h"

void Systick_Init(void);
uint32_t micros(void);
uint32_t millis(void);
void delay_ms(uint32_t nTime);
void delay_us(uint32_t nTime);