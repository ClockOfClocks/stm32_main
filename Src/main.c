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
  UART_Init();
  
/*
  // Test tasks
  struct AxisTask task0;
  task0.type = AXIS_TASK_TYPE_CALIBRATION;
  queue_push(ax1.queue, &task0.n);
  
  struct AxisTask task;
  task.type = AXIS_TASK_TYPE_MOVE;
  task.degree = 15;
  task.speed = 10;
  task.relative = false;
  queue_push(ax1.queue, &task.n);
  
  struct AxisTask task2;
  task2.type = AXIS_TASK_TYPE_MOVE;
  task2.degree = -30;
  task2.speed = 20;
  task2.relative = false;
  queue_push(ax1.queue, &task2.n);
  
  struct AxisTask task3;
  task3.type = AXIS_TASK_TYPE_WAIT;
  task3.wait_ms = 1000;
  queue_push(ax1.queue, &task3.n);

  struct AxisTask task4;
  task4.type = AXIS_TASK_TYPE_MOVE;
  task4.degree = 30;
  task4.speed = 30;
  task4.relative = true;
  queue_push(ax1.queue, &task4.n);

  struct AxisTask task5;
  task5.type = AXIS_TASK_TYPE_WAIT;
  task5.wait_ms = 2000;
  queue_push(ax1.queue, &task5.n);

  struct AxisTask task6;
  task6.type = AXIS_TASK_TYPE_MOVE;
  task6.degree = 30;
  task6.speed = 2;
  task6.relative = false;
  queue_push(ax1.queue, &task6.n);
*/

  // queue_remove_all(ax1.queue);

  // Ready for tasks
  ax1.state = AXIS_STATUS_IDLE;

	while(1)
	{
    // Do nothing
    if ((GPIOB->IDR & GPIO_IDR_IDR8) == GPIO_IDR_IDR8){
      GPIOC->BSRR = GPIO_BSRR_BS13;
    }else{
      GPIOC->BSRR = GPIO_BSRR_BR13;
    }
	}
}

void Axis_Init (void){
  ax1.state = AXIS_STATUS_INIT;
  ax1.pointer_position = INITIAL_POINTER_POSITION;
  ax1.target_pointer_position = INITIAL_POINTER_POSITION;
  ax1.direction = true;

  ax1.sin_pwm_channel1 = & TIM2->CCR1;
  ax1.sin_pwm_channel2 = & TIM2->CCR2;
  ax1.cos_pwm_channel1 = & TIM2->CCR3;
  ax1.cos_pwm_channel2 = & TIM2->CCR4; 
}

void TIM2_IRQHandler(void)
{
  TIM2->SR &= ~TIM_SR_UIF; // clear interruption flag

  switch (ax1.state)
  {
  case AXIS_STATUS_IDLE:
      extract_task(&ax1);        
    break;

  case AXIS_STATUS_WAIT:
    if(millis() >= ax1.wait_until_ms){
      // continue
      ax1.state = AXIS_STATUS_IDLE;
    }
    break;

    case AXIS_STATUS_MOVE:
    case AXIS_STATUS_CALIBRATION:    
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

      if(AXIS_STATUS_CALIBRATION == ax1.state){
        if ((GPIOB->IDR & GPIO_IDR_IDR8) != GPIO_IDR_IDR8){
          if(ax1.calibration_state == AXIS_CALIBRATION_SEARCH) {
            ax1.calibration_pointer_position = ax1.pointer_position;
            ax1.calibration_state = AXIS_CALIBRATION_DETECTED;
          }
        }else {
          if(ax1.calibration_state == AXIS_CALIBRATION_INIT) {
            // do not track case when magnet detected in the beginning of calibration
            ax1.calibration_state = AXIS_CALIBRATION_SEARCH;
          }else if(ax1.calibration_state == AXIS_CALIBRATION_DETECTED) {

            // initial position + position of magnet center
            ax1.pointer_position = INITIAL_POINTER_POSITION + (ax1.pointer_position - ax1.calibration_pointer_position ) / 2;
            ax1.calibration_state = AXIS_CALIBRATION_FINISHED;

            // if no tasks in the queue, move to home position after calibratin finish
            if(queue_size(ax1.queue) == 0){
              struct AxisTask task;
              task.type = AXIS_TASK_TYPE_MOVE;
              task.degree = 0;
              task.speed = 10;
              task.relative = false;
              queue_push(ax1.queue, &task.n);
            }
            // pick next task
            ax1.state = AXIS_STATUS_IDLE;
          }
        }
      }
    break;

  default:
    break;
  }
}

void USART1_IRQHandler (void){
	
	if (USART1->SR & USART_SR_RXNE){		
		USART1->SR &= ~USART_SR_RXNE;
		
  char input = USART1->DR;
  float x = (float)(input - '0');

  //USART1_Send(input);

  struct AxisTask task;
  task.type = AXIS_TASK_TYPE_MOVE;
  task.degree = x;
  task.speed = 10;
  task.relative = false;
  queue_push(ax1.queue, &task.n);
	}	
}
