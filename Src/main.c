#include "main.h"
#include "timing.h"

void RCC_Init (void);
void GPIO_Init (void);
void PWM_Init (void);
void Systick_Init (void);

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

#define COS_OFFSET 64

#define INTERRUPTION_FREQ 23437.5 // 72MHz / 3 (PSC) / 1024 (ARR) = 23437.5 Hz;

// (12-bit global position) (8-bit sin table position) (12-bit precision)
// 12-bit position is 2^12 / 2 / (720/4) ~= 11.37 revolutions in each direction
#define INITIAL_POINTER_POSITION (1 << 31) // ( ( (1 << 12) / 2 ) << 20 )

#define PERIODS_PER_REVOLUTION 180 // 720 (steps per revolution) / 4

#define ONE_PERIOD_POINTER_DIFF (1 << 20) // 12 + 8 bit

#define ONE_DEGREE_POINTER_DIFF (ONE_PERIOD_POINTER_DIFF * PERIODS_PER_REVOLUTION / 360)

float speed_degree_per_second = 20;

uint32_t pointer_position = INITIAL_POINTER_POSITION;

// Calculable values
uint32_t target_pointer_position;
int16_t pointer_diff_per_interruption;

int main(void){		
  pointer_diff_per_interruption = (speed_degree_per_second * ONE_DEGREE_POINTER_DIFF) / INTERRUPTION_FREQ;
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


void RCC_Init (void){
	
	RCC->CR |= ((uint32_t)RCC_CR_HSEON); 												// Enable HSE
	while (!(RCC->CR & RCC_CR_HSERDY));													// Ready start HSE		
	
	FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;					// Cloclk Flash memory
		
  // Clear
	RCC->CFGR &= ~RCC_CFGR_PLLSRC;															// clearn PLLSRC bits
	RCC->CFGR &= ~RCC_CFGR_PLLMULL;               							// clear PLLMULL bits

  // Setup
  RCC->CFGR |= RCC_CFGR_PLLSRC; 											        // source HSE
	RCC->CFGR &= ~RCC_CFGR_PLLXTPRE; 								            // source HSE/1 = 8 MHz
	RCC->CFGR |= RCC_CFGR_PLLMULL9; 														// PLL x9: clock = 8 MHz * 9 = 72 MHz
	
  RCC->CFGR |= RCC_CFGR_HPRE_DIV1;														// AHB = SYSCLK/1
	RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;														// APB1 = HCLK/2 = 72/2 = 36MHz (why? it should be 32MHz max)
	RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;														// APB2 = HCLK/1

	RCC->CR |= RCC_CR_PLLON;                      							// enable PLL
	while((RCC->CR & RCC_CR_PLLRDY) == 0) {}      							// wait till PLL is ready
	
	RCC->CFGR &= ~RCC_CFGR_SW;                   							 	// clear SW bits
  RCC->CFGR |= RCC_CFGR_SW_PLL;                 							// select source SYSCLK = PLL
	while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_1) {} 			// wait till PLL is used	
}

void GPIO_Init (void){
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; // enable clock for port A

  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; // enable clock for port B
  
  GPIOB->CRH &= ~GPIO_CRH_CNF8;
	GPIOB->CRH |= GPIO_CRH_MODE8_0;

  GPIOB->CRH &= ~GPIO_CRH_CNF9;
	GPIOB->CRH |= GPIO_CRH_MODE9_0;


  GPIOB->CRH &= ~GPIO_CRH_CNF10;
	GPIOB->CRH |= GPIO_CRH_MODE10_0;

  GPIOB->CRH &= ~GPIO_CRH_CNF11;
	GPIOB->CRH |= GPIO_CRH_MODE11_0;
}

void PWM_Init (void){
  // Enable clock on A port
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;   
  // Enable clock for alternative push-pull output (required for pwm)
  RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;	

  //ch1 -PA0
   GPIOA->CRL |= GPIO_CRL_MODE0;  //50Mhz
   GPIOA->CRL &= ~GPIO_CRL_CNF0; //clear CNF[1:0] for PA0 
   GPIOA->CRL |= GPIO_CRL_CNF0_1; //output Push-Pull in alternative function mode
   //ch2 - PA1
   GPIOA->CRL |= GPIO_CRL_MODE1;  //50Mhz
   GPIOA->CRL &= ~GPIO_CRL_CNF1; //clear CNF[1:0] for PA1 
   GPIOA->CRL |= GPIO_CRL_CNF1_1; //output Push-Pull in alternative function mode

   /*
   //ch3 - PA2
   GPIOA->CRL |= GPIO_CRL_MODE2;  //50Mhz
   GPIOA->CRL &= ~GPIO_CRL_CNF2; //clear CNF[1:0] for PA2 
   GPIOA->CRL |= GPIO_CRL_CNF2_1; //output Push-Pull in alternative function mode
   //ch4 - PA3
   GPIOA->CRL |= GPIO_CRL_MODE3;  //50Mhz
   GPIOA->CRL &= ~GPIO_CRL_CNF3; //clear CNF[1:0] for PA3 
   GPIOA->CRL |= GPIO_CRL_CNF3_1; //output Push-Pull in alternative function mode
   */

   //TIM2 Settings
   RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
   TIM2->CR1 |= TIM_CR1_ARPE;  //autorelode mode
   TIM2->CCMR1 |= TIM_CCMR1_OC1PE | TIM_CCMR1_OC2PE;  //Output Compare Preload enable
   TIM2->CCMR2 |= TIM_CCMR2_OC3PE | TIM_CCMR2_OC4PE;
   TIM2->PSC = 3 - 1; // div for clock: F = SYSCLK / [PSC + 1] // every 3rd tick
   
   TIM2->ARR  = 1024; // 72MHz / 3 (PSC) / 1024 (ARR) = 23437.5 Hz
   TIM2->CCR1 = 1; //ch1 1duty cycle
   TIM2->CCR2 = 100; //ch2 1duty cycle
   //TIM2->CCR3 = 500; //ch3 duty cycle
   //TIM2->CCR4 = 80; //ch4 duty cycle
   
   //TIM2->CCER |= TIM_CCER_CC2P;  //polarity of output signal
   //Capture/Compare 2 output enable
   TIM2->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;
   //Output Compare Mode - 110 - PWM mode 1
   TIM2->CCMR1 |= (TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1); 
   TIM2->CCMR1 |= (TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1);
   TIM2->CCMR2 |= (TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1);
   TIM2->CCMR2 |= (TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1);

   TIM2->DIER |= TIM_DIER_UIE; // Enable tim2 interrupt

   //start counting
   TIM2->CR1 |= TIM_CR1_CEN;   

   NVIC_EnableIRQ(TIM2_IRQn); // enable interrupt
}

void TIM2_IRQHandler(void)
{
    TIM2->SR &= ~TIM_SR_UIF; // очистка флага прерывания

    pointer_position += pointer_diff_per_interruption;

    // clear 12 most bits and shift value right to trim 12 lower bits
    uint8_t sin_pointer = ( pointer_position & ~( ( ( 1 << 12 ) - 1 ) << 20 ) ) >> 12;
    uint8_t cos_pointer = sin_pointer + COS_OFFSET;

    int16_t sin_value = sin_data[ sin_pointer ];
    int16_t cos_value = sin_data[ cos_pointer ];
    
    if(sin_value < 0){
      GPIOB->BSRR = ( GPIO_BSRR_BR8 | GPIO_BSRR_BS9);
      TIM2->CCR1 = -sin_value;
    }else{
      // Positive or zero
      GPIOB->BSRR = ( GPIO_BSRR_BS8 | GPIO_BSRR_BR9);
      TIM2->CCR1 = sin_value;
    }

    if(cos_value < 0){
      GPIOB->BSRR = ( GPIO_BSRR_BR10 | GPIO_BSRR_BS11);
      TIM2->CCR2 = -cos_value;
    }else{
      // Positive or zero
      GPIOB->BSRR = ( GPIO_BSRR_BS10 | GPIO_BSRR_BR11);
      TIM2->CCR2 = cos_value;
    }
}
