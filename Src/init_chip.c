#include "main.h"

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
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;														// APB1 = HCLK/2 = 72/2 = 36MHz
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

    // Input pull-up for Hall sensors, cnf1=1, cnf0=0, mode1,0=0
    GPIOB->CRH &= ~(GPIO_CRH_CNF8 | GPIO_CRH_MODE8);
    GPIOB->CRH |= GPIO_CRH_CNF8_1;
    GPIOB->ODR |= GPIO_ODR_ODR8; //enable pull-up

    GPIOB->CRH &= ~(GPIO_CRH_CNF9 || GPIO_CRH_MODE9);
    GPIOB->CRH |= GPIO_CRH_CNF9_1;
    GPIOB->ODR |= GPIO_ODR_ODR9;

    // Setup on-board led to test hall sensor
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN; // enable clock for port B
    GPIOC->CRH &= ~GPIO_CRH_CNF13;
    GPIOC->CRH |= GPIO_CRH_MODE13_0;
}

void PWM_Init (void){
    // Enable clock on A port
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;   
    // Enable clock for alternative push-pull output (required for pwm)
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;	

    //ch1 - PA0
    GPIOA->CRL |= GPIO_CRL_MODE0;  //50Mhz
    GPIOA->CRL &= ~GPIO_CRL_CNF0; //clear CNF[1:0] for PA0 
    GPIOA->CRL |= GPIO_CRL_CNF0_1; //output Push-Pull in alternative function mode
    //ch2 - PA1
    GPIOA->CRL |= GPIO_CRL_MODE1;  //50Mhz
    GPIOA->CRL &= ~GPIO_CRL_CNF1; //clear CNF[1:0] for PA1 
    GPIOA->CRL |= GPIO_CRL_CNF1_1; //output Push-Pull in alternative function mode
    //ch3 - PA2
    GPIOA->CRL |= GPIO_CRL_MODE2;  //50Mhz
    GPIOA->CRL &= ~GPIO_CRL_CNF2; //clear CNF[1:0] for PA2 
    GPIOA->CRL |= GPIO_CRL_CNF2_1; //output Push-Pull in alternative function mode
    //ch4 - PA3
    GPIOA->CRL |= GPIO_CRL_MODE3;  //50Mhz
    GPIOA->CRL &= ~GPIO_CRL_CNF3; //clear CNF[1:0] for PA3 
    GPIOA->CRL |= GPIO_CRL_CNF3_1; //output Push-Pull in alternative function mode
    
    //TIM2 Settings
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->CR1 |= TIM_CR1_ARPE;  //autorelode mode
    TIM2->CCMR1 |= TIM_CCMR1_OC1PE | TIM_CCMR1_OC2PE;  //Output Compare Preload enable
    TIM2->CCMR2 |= TIM_CCMR2_OC3PE | TIM_CCMR2_OC4PE;
    TIM2->PSC = 3 - 1; // div for clock: F = SYSCLK / [PSC + 1] // every 3rd tick

    TIM2->ARR  = 1024; // 72MHz / 3 (PSC) / 1024 (ARR) = 23437.5
    TIM2->CCR1 = 1; //ch1 1duty cycle
    TIM2->CCR2 = 1; //ch2 1duty cycle
    TIM2->CCR3 = 1; //ch3 duty cycle
    TIM2->CCR4 = 1; //ch4 duty cycle

    //TIM2->CCER |= TIM_CCER_CC2P;  //polarity of output signal
    //Capture/Compare 2 output enable
    TIM2->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;
    //Output Compare Mode - 110 - PWM mode 1
    TIM2->CCMR1 |= (TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1); 
    //Output Compare Mode - 111 - PWM mode 2
    TIM2->CCMR1 |= (TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_0);
    //Output Compare Mode - 110 - PWM mode 1
    TIM2->CCMR2 |= (TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1);
    //Output Compare Mode - 111 - PWM mode 2
    TIM2->CCMR2 |= (TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4M_0);

    TIM2->DIER |= TIM_DIER_UIE; // Enable tim2 interrupt

    //start counting
    TIM2->CR1 |= TIM_CR1_CEN;   

    NVIC_EnableIRQ(TIM2_IRQn); // enable interrupt
}

void TIM4_Init (void){
    //TIM4 Settings
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;

    TIM4->CR1 |= TIM_CR1_ARPE;  //autorelode mode
    // TIM4->CCMR1 |= TIM_CCMR1_OC1PE | TIM_CCMR1_OC2PE;  //Output Compare Preload enable
    // TIM4->CCMR2 |= TIM_CCMR2_OC3PE | TIM_CCMR2_OC4PE;
    TIM4->PSC = 36 - 1; // div for clock: F = SYSCLK / [PSC + 1] // every 3rd tick

    TIM4->ARR  = 100; // 36MHz / 3 (PSC) / 1024 (ARR) = 23437.5 Hz
    TIM4->CCR1 = 1; //ch1 1duty cycle

    TIM4->DIER |= TIM_DIER_UIE; // Enable tim2 interrupt

    //start counting
    TIM4->CR1 |= TIM_CR1_CEN;   

    NVIC_EnableIRQ(TIM4_IRQn); // enable interrupt
}


void UART_Init (void){
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

    // tx setup
    GPIOA->CRH |= GPIO_CRH_CNF9_1;
    GPIOA->CRH &= ~GPIO_CRH_CNF9_0; //
    GPIOA->CRH |= GPIO_CRH_MODE9;

    // rx setup
    GPIOA->CRH |= GPIO_CRH_CNF10_0;
    GPIOA->CRH &= ~GPIO_CRH_CNF10_1; // 
    GPIOA->CRH &= ~GPIO_CRH_MODE10;

    // tx/rx switch output
    GPIOA->CRH &= ~GPIO_CRH_CNF11;
    GPIOA->CRH |= GPIO_CRH_MODE11;

    // baudrate
    USART1->BRR = 0x1D4C; // 9600
    USART1->CR1 |= USART_CR1_TE; // enable TX
    USART1->CR1 |= USART_CR1_RE; // enable RX
    USART1->CR1 |= USART_CR1_UE; // enable usart

    // USART1->CR1 |= USART_CR1_RXNEIE; // enable interruption on receive
    
    NVIC_EnableIRQ(USART1_IRQn);

    // tx/rx switch default: rx
    //GPIOA->BSRR = GPIO_BSRR_BR11;

    serialDisableTransfer();
}
