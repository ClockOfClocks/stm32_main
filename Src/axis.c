#include "axis.h"

int16_t sin_data[256] = // 256 values per 2pi, +\- 1024
{0,25,50,75,100,125,150,175,199,224,248,273,297,321,344,368,391
,414,437,460,482,504,526,547,568,589,609,629,649,668,687,706,724
,741,758,775,791,807,822,837,851,865,878,890,903,914,925,936,946
,955,964,972,979,986,993,999,1004,1008,1012,1016,1019,1021,1022,1023,1024
,1023,1022,1021,1019,1016,1012,1008,1004,999,993,986,979,972,964,955,946
,936,925,914,903,890,878,865,851,837,822,807,791,775,758,741,724
,706,687,668,649,629,609,589,568,547,526,504,482,460,437,414,391
,368,344,321,297,273,248,224,199,175,150,125,100,75,50,25,0
,-25,-50,-75,-100,-125,-150,-175,-199,-224,-248,-273,-297,-321,-344,-368,-391
,-414,-437,-460,-482,-504,-526,-547,-568,-589,-609,-629,-649,-668,-687,-706,-724
,-741,-758,-775,-791,-807,-822,-837,-851,-865,-878,-890,-903,-914,-925,-936,-946
,-955,-964,-972,-979,-986,-993,-999,-1004,-1008,-1012,-1016,-1019,-1021,-1022,-1023,-1024
,-1023,-1022,-1021,-1019,-1016,-1012,-1008,-1004,-999,-993,-986,-979,-972,-964,-955,-946
,-936,-925,-914,-903,-890,-878,-865,-851,-837,-822,-807,-791,-775,-758,-741,-724
,-706,-687,-668,-649,-629,-609,-589,-568,-547,-526,-504,-482,-460,-437,-414,-391
,-368,-344,-321,-297,-273,-248,-224,-199,-175,-150,-125,-100,-75,-50,-25};


void update_axis_sin_cos(struct Axis *axis){
    // clear 12 most bits and shift value right to trim 12 lower bits
    uint8_t sin_pointer = ( axis->pointer_position & ~( ( ( 1 << 12 ) - 1 ) << 20 ) ) >> 12;
    uint8_t cos_pointer = sin_pointer + COS_OFFSET;

    int16_t sin_value = sin_data[ sin_pointer ];
    int16_t cos_value = sin_data[ cos_pointer ];
    
    if(sin_value < 0){
      *axis->sin_gpio_port_bsrr = axis->sin_cos_direction[1];
      *axis->sin_pwm_pointer = -sin_value;
    }else{
      // Positive or zero
      *axis->sin_gpio_port_bsrr = axis->sin_cos_direction[0];
      *axis->sin_pwm_pointer = sin_value;
    }

    if(cos_value < 0){
      *axis->cos_gpio_port_bsrr = axis->sin_cos_direction[3];
      *axis->cos_pwm_pointer = -cos_value;
    }else{
      // Positive or zero
      *axis->cos_gpio_port_bsrr = axis->sin_cos_direction[2];
      *axis->cos_pwm_pointer = cos_value;
    }
}

void set_axis_speed(struct Axis *axis, float degreePerSecond){
  axis->pointer_diff_per_interruption = (degreePerSecond * ONE_DEGREE_POINTER_DIFF) / INTERRUPTION_FREQ;
  // (X degress/s * (1 << 20) * (720/4) / 360) / 23437.5  ~ X * 5.5924
}

void move_axis_to(struct Axis *axis, float degree, bool relative){
    int32_t diff = (ONE_DEGREE_POINTER_DIFF * degree);

    if(relative){
        axis->target_pointer_position = axis->pointer_position + diff;
    }else{
        axis->target_pointer_position = INITIAL_POINTER_POSITION + diff;
    }
    axis->direction = axis->target_pointer_position > axis->pointer_position;
}

void extract_task(struct Axis *axis){
    if(queue_size(axis->queue) == 0){
      axis->state = AXIS_STATUS_IDLE;
    }else{
      struct AxisTask *task = queue_entry(queue_peek(axis->queue), struct AxisTask, n);
      set_axis_speed(axis, task->speed);
      move_axis_to(axis, task->degree, task->relative);
      
      // Remove task from queue
      queue_pop(axis->queue);

      // Let's move axis 
      axis->state = AXIS_STATUS_MOVE;
    }
}