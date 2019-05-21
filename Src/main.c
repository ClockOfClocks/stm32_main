#include "main.h"

struct Axis ax1;
 
int main(void){
  
  Axis_Init();
  Queue q;
  ax1.queue = &q;
  queue_init(ax1.queue);
  
  RCC_Init();
  Systick_Init();
	GPIO_Init();
	PWM_Init();

  // Test tasks
  struct AxisTask task;
  task.degree = 15;
  task.speed = 10;
  task.relative = false;
  queue_push(ax1.queue, &task.n);
  
  struct AxisTask task2;
  task2.degree = -30;
  task2.speed = 20;
  task2.relative = false;
  queue_push(ax1.queue, &task2.n);
  
  struct AxisTask task3;
  task3.degree = 30;
  task3.speed = 30;
  task3.relative = true;
  queue_push(ax1.queue, &task3.n);

  // Ready for tasks
  ax1.state = AXIS_STATUS_IDLE;

	while(1)
	{
    // Do nothing
	}
}

void Axis_Init (void){
  ax1.state = AXIS_STATUS_INIT;
  ax1.pointer_position = INITIAL_POINTER_POSITION;
  ax1.target_pointer_position = INITIAL_POINTER_POSITION;
  ax1.direction = true;

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

  switch (ax1.state)
  {
  case AXIS_STATUS_IDLE:
      extract_task(&ax1);        
    break;

    case AXIS_STATUS_MOVE:
      if(ax1.direction){
        ax1.pointer_position += ax1.pointer_diff_per_interruption;
        update_axis_sin_cos(& ax1);
        if(ax1.target_pointer_position <= ax1.pointer_position){
          // finished    
          ax1.state = AXIS_STATUS_IDLE;
        }
      } else {
          ax1.pointer_position -= ax1.pointer_diff_per_interruption;
          update_axis_sin_cos(& ax1);
        if(ax1.target_pointer_position >= ax1.pointer_position){
          // finished
          ax1.state = AXIS_STATUS_IDLE;
        }
      }
    break;

  default:
    break;
  }
}

