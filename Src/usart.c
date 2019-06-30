#include "usart.h"

void USART1_Send (char chr){
	while (!(USART1->SR & USART_SR_TC));
	GPIOA->BSRR = GPIO_BSRR_BS11;
	USART1->DR = chr;
    
    while (!(USART1->SR & USART_SR_TC));
	GPIOA->BSRR = GPIO_BSRR_BR11;
}

void USART1_Send_String (char* str){
	uint8_t i = 0;
	
	while(str[i]){
    	USART1_Send (str[i++]);
    }
}
