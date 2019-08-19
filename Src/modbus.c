#include "modbus.h"
#include "usart.h"

void serialEnableTransfer(){
    // tx enable
    GPIOA->BSRR = GPIO_BSRR_BS11;

    // disable interruption on receive
    USART1->CR1 &= ~USART_CR1_RXNEIE;

    // enable interruption on transfer complete
    USART1->CR1 |= USART_CR1_TCIE;

    // led on
    GPIOC->BSRR = GPIO_BSRR_BR13;
}
void serialDisableTransfer(){
    // tx disable
    GPIOA->BSRR = GPIO_BSRR_BR11;

    // enable interruption on receive
    USART1->CR1 |= USART_CR1_RXNEIE;

    // disable interruption on transfer complete
    USART1->CR1 &= ~USART_CR1_TCIE;

    // led off
    GPIOC->BSRR = GPIO_BSRR_BS13;
}

void TIM4_IRQHandler(void) {
    TIM4->SR &= ~TIM_SR_UIF; // clear interruption flag

    // если наш таймер больше уставки задержки и есть символы то есть gap -перерыв в посылке 
    // и можно ее обрабатывать
    if ((uart1.rxtimer++ > uart1.delay) & (uart1.rxcnt > 1)) {
        uart1.rxgap = 1;
    } else {
        uart1.rxgap = 0;
    }
}

void USART1_IRQHandler(void) {

    // if (USART1->SR & USART_SR_RXNE){		
    //     USART1->SR &= ~USART_SR_RXNE;
    //     char input = USART1->DR;
    //     USART1_Send(input);
    //     USART1_Send_String(" exfff\n");
    // }

    // return;

    //Receive Data register not empty interrupt
    if (USART1->SR & USART_SR_RXNE) {
        USART1->SR &= ~USART_SR_RXNE;
        uart1.rxtimer = 0;

        if (uart1.rxcnt > (BUF_SZ - 2)) {
            uart1.rxcnt = 0;
        }
        uart1.buffer[uart1.rxcnt++] = USART1->DR;
    }

    //Transmission complete interrupt
    if (USART1->SR & USART_SR_TC) {
        USART1->SR &= ~USART_SR_TC;

        if (uart1.txcnt < uart1.txlen) {
            // USART1->DR = (Data & (uint16_t)0x01FF); // from hal sources
            USART1->DR = uart1.buffer[uart1.txcnt++];
        } else {
            // transfer completed
            uart1.txlen = 0;

            serialDisableTransfer();
        }
    }
}

void net_tx3(UART_DATA *uart) {
    if ((uart->txlen > 0) & (uart->txcnt == 0)) {
        serialEnableTransfer();
        //USART1_Send_String("XUILALALAL");
        USART1->DR = uart->buffer[uart->txcnt++];
    }
}

void MODBUS_SLAVE(UART_DATA *MODBUS) {
    unsigned int tmp;

    //recive and checking rx query
    if ((MODBUS->buffer[0] != 0) & (MODBUS->rxcnt > 5) &
        ((MODBUS->buffer[0] == SET_PAR[0]) | (MODBUS->buffer[0] == 255))) {
        tmp = Crc16(MODBUS->buffer, MODBUS->rxcnt - 2);

        if ((MODBUS->buffer[MODBUS->rxcnt - 2] == (tmp & 0x00FF)) & (MODBUS->buffer[MODBUS->rxcnt - 1] == (tmp >> 8))) {


            //choosing function
            switch (MODBUS->buffer[1]) {
                case 3:
                    TX_03_04(MODBUS);
                    break;

                case 4:
                    TX_03_04(MODBUS);
                    break;

                case 6: // write one register
                case 16: // write multiple registers
                    TX_06(MODBUS);
                    break;

                default:
                    //illegal operation
                    TX_EXCEPTION(MODBUS, 0x01);
            }

            //adding CRC16 to reply
            tmp = Crc16(MODBUS->buffer, MODBUS->txlen - 2);
            MODBUS->buffer[MODBUS->txlen - 2] = tmp;
            MODBUS->buffer[MODBUS->txlen - 1] = tmp >> 8;
            MODBUS->txcnt = 0;
        }
    }

    MODBUS->rxgap = 0;
    MODBUS->rxcnt = 0;
    MODBUS->rxtimer = 0xFFFF;
}

void TX_03_04(UART_DATA *MODBUS) {
    unsigned int tmp, tmp1;
    unsigned int m = 0, n = 0;
    int tmp_val, tmp_val_pos;

    //MODBUS->buffer[0] =SET_PAR[0]; // adress - stays a same as in received
    //MODBUS->buffer[1] = 3; //query type - - stay a same as in recived
    //MODBUS->buffer[2] = data byte count

    //2-3  - starting address
    tmp = ((MODBUS->buffer[2] << 8) + MODBUS->buffer[3]);

    //4-5 - number of registers
    tmp1 = ((MODBUS->buffer[4] << 8) + MODBUS->buffer[5]);

    //default answer length if error
    n = 3;

    if ((((tmp + tmp1) < OBJ_SZ) & (tmp1 < MODBUS_WRD_SZ + 1))) {

        for (m = 0; m < tmp1; m++) {
            tmp_val = 13; //res_table.regs[m + tmp];

            if (tmp_val < 0) {
                tmp_val_pos = tmp_val;
                MODBUS->buffer[n] = (tmp_val_pos >> 8) | 0b10000000;
                MODBUS->buffer[n + 1] = tmp_val_pos;
            } else {
                MODBUS->buffer[n] = tmp_val >> 8;
                MODBUS->buffer[n + 1] = tmp_val;
            }
            n = n + 2;
        }

        MODBUS->buffer[2] = m * 2; //byte count
        MODBUS->txlen = m * 2 + 5; //responce length // 5 = (addr=1) + (func=1) + (count of data bytes=1) + (crc=2)

    } else {
        //exception illegal data adress 0x02
        TX_EXCEPTION(MODBUS, 0x02);
    }

}

//*******************************************************
//Writing
//*******************************************************
void TX_06(UART_DATA *MODBUS) {
    unsigned int tmp;
    union {
        int16_t reg16;
        int8_t byte8[2];
    } buf;

    //MODBUS[0] =SET_PAR[0]; // adress - stays a same as in recived
    //MODBUS[1] = 6; //query type - - stay a same as in recived

    //2-3  - adress   , 4-5 - value

    tmp = ((MODBUS->buffer[2] << 8) + MODBUS->buffer[3]); //adress

    //MODBUS->buffer[2]  - byte count a same as in rx query

    if (tmp < OBJ_SZ) {
        MODBUS->txlen = MODBUS->rxcnt; //responce length
        buf.reg16 = (MODBUS->buffer[4] << 8) + MODBUS->buffer[5];

        toMov = (float)buf.reg16;

        // if (res_table.regs[tmp] != buf.reg16) {
        //     res_table.regs[tmp] = buf.reg16;
        //     //only adress - 0 register writing to eeprom
        //     // if (tmp == 0) {
        //     //     I2C_EE_ByteWrite((uint8_t) buf.byte8[0], tmp * 2);
        //     //     I2C_EE_ByteWrite((uint8_t) buf.byte8[1], tmp * 2 + 1);
        //     //     SET_PAR[0] = res_table.regs[0];
        //     // }
        // }
    } else {
        //illegal data
        TX_EXCEPTION(MODBUS, 0x02);
    }

}



//modbus exception - illegal data=01 ,adress=02 etc 
void TX_EXCEPTION(UART_DATA *MODBUS, unsigned char error_type) {
    //illegal operation
    MODBUS->buffer[2] = error_type; //exception
    MODBUS->txlen = 5; //responce length
}

//*********************************************************************
//CRC16 for Modbus Calculation
//*********************************************************************
unsigned int Crc16(unsigned char *ptrByte, int byte_cnt) {
    unsigned int w = 0;
    char shift_cnt;

    if (ptrByte) {
        w = 0xffffU;
        for (; byte_cnt > 0; byte_cnt--) {
            w = (unsigned int) ((w / 256U) * 256U + ((w % 256U) ^ (*ptrByte++)));
            for (shift_cnt = 0; shift_cnt < 8; shift_cnt++) {
                if ((w & 0x1) == 1)
                    w = (unsigned int) ((w >> 1) ^ 0xa001U);
                else
                    w >>= 1;
            }
        }
    }
    return w;
}
