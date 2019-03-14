#include "stdint.h"
#include "timing.h"

volatile uint32_t Millis;

void Systick_Init(void)
{
    uint32_t returnCode;
    returnCode = SysTick_Config(SystemCoreClock / 1000);      /* Configure SysTick to generate an interrupt every millisecond */
    
      if (returnCode != 0)  {                                   /* Check return code for errors */
        // Error Handling 
            while (1);
      }

    NVIC_SetPriority(SysTick_IRQn, 0);//i want to make sure systick has highest priority amount all other interrupts

    Millis = 0; //reset Millis
}

void SysTick_Handler(void) {
    Millis++;
}

uint32_t micros(void)
{
    // = Millis*1000+(SystemCoreClock/1000-SysTick->VAL)/72;
    return  Millis * 1000 + 1000 - SysTick->VAL / 72;
}

uint32_t millis(void)
{
    return Millis;
}

void delay_ms(uint32_t nTime)
{
    uint32_t curTime = Millis;
    while( nTime - ( Millis - curTime ) > 0);
}

void delay_us(uint32_t nTime)
{
    uint32_t curTime = micros();
    while( nTime - ( micros() - curTime ) > 0);
}