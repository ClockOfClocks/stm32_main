#include "stdint.h"
#include "stdbool.h"

#define COS_OFFSET 64

#define INTERRUPTION_FREQ 23437.5 // 72MHz / 3 (PSC) / 1024 (ARR) = 23437.5 Hz;

#define PERIODS_PER_REVOLUTION 180 // 720 (steps per revolution) / 4

#define ONE_DEGREE_POINTER_DIFF ( (1 << 20) * PERIODS_PER_REVOLUTION / 360 ) // one_period_pointer_diff * periods_per_revolution / 360

// (12-bit global position) (8-bit sin table position) (12-bit precision)
// 12-bit position is 2^12 / 2 / (720/4) ~= 11.37 revolutions in each direction
#define INITIAL_POINTER_POSITION (1 << 31) // ( ( (1 << 12) / 2 ) << 20 )

struct Axis {
   uint32_t pointer_position;
   // uint8_t state;
   // bool direction; // true – positive
   float speed_degree_per_second;
   int16_t pointer_diff_per_interruption; // speed
   // uint32_t target_pointer_position; 

   volatile uint32_t *sin_pwm_pointer;
   volatile uint32_t *cos_pwm_pointer;
   volatile uint32_t *sin_gpio_port_bsrr;
   volatile uint32_t *cos_gpio_port_bsrr;
   uint32_t sin_cos_direction[4]; // [+sin, -sin, +cos, -cos]
};

void update_axis_sin_cos(struct Axis *axis);
