#include "main.h"

struct Axis ax1;

int main(void){		
  Axis_Init();
  
  ax1.speed_degree_per_second = 10;
  // ax1.direction = true;
  ax1.pointer_diff_per_interruption = (ax1.speed_degree_per_second * ONE_DEGREE_POINTER_DIFF) / INTERRUPTION_FREQ;
  // (X degress/s * (1 << 20) * (720/4) / 360) / 23437.5  ~ X * 5.5924

  RCC_Init();
  Systick_Init();
	GPIO_Init();
	PWM_Init();
		
	while(1)
	{
    // Do nothing
	}
}

void Axis_Init (void){
  ax1.pointer_position = INITIAL_POINTER_POSITION;
  ax1.sin_pwm_pointer = & TIM2->CCR1;
  ax1.cos_pwm_pointer = & TIM2->CCR2;
  ax1.sin_cos_direction[0] = ( GPIO_BSRR_BS8 | GPIO_BSRR_BR9 );
  ax1.sin_cos_direction[1] = ( GPIO_BSRR_BR8 | GPIO_BSRR_BS9 );
  ax1.sin_cos_direction[2] = ( GPIO_BSRR_BS10 | GPIO_BSRR_BR11 );
  ax1.sin_cos_direction[3] = ( GPIO_BSRR_BR10 | GPIO_BSRR_BS11 );
  ax1.sin_gpio_port_bsrr = & GPIOB->BSRR;
  ax1.cos_gpio_port_bsrr = & GPIOB->BSRR;
}

void TIM2_IRQHandler(void)
{
    TIM2->SR &= ~TIM_SR_UIF; // clear interruption flag

    ax1.pointer_position += ax1.pointer_diff_per_interruption;
    update_axis_sin_cos(& ax1);
}

