#ifndef MODBUS_H
#define MODBUS_H

#include "stm32f103xb.h"

#define OBJ_SZ 123 //это количество объектов
#define SETUP 4 //это просто количество данных в массиве 0-элемент которого означает адрес

//PARAMETERRS ARRAY 0 PARAMETER = MODBUS ADDRESS
unsigned char SET_PAR[SETUP]; // 0-элемент это адрес

float toMov;

//OBJECT ARRAY WHERE READING AND WRITING OCCURS
union {
    int16_t regs[OBJ_SZ];
    int8_t bytes[OBJ_SZ * 2];
} res_table;

//buffer uart
#define BUF_SZ 256 //размер буфера
#define MODBUS_WRD_SZ (BUF_SZ-5)/2 //максимальное количество регистров в ответе

//uart structure
typedef struct {
    unsigned char buffer[BUF_SZ]; //буфер
    unsigned int rxtimer; //этим мы считаем таймаут
    unsigned char rxcnt; //количество принятых символов
    unsigned char txcnt; //количество переданных символов
    unsigned char txlen; //длина посылки на отправку
    unsigned char rxgap; //окончание приема
    unsigned char delay; //задержка
} UART_DATA;

UART_DATA uart1; //структуры для соответсвующих усартов

void net_tx3(UART_DATA *uart);

void MODBUS_SLAVE(UART_DATA *MODBUS); //функция обработки модбас и формирования ответа

unsigned int Crc16(unsigned char *ptrByte, int byte_cnt);

void TX_03_04(UART_DATA *MODBUS);

void TX_06(UART_DATA *MODBUS);

void TX_EXCEPTION(UART_DATA *MODBUS, unsigned char error_type);

void serialEnableTransfer(void);
void serialDisableTransfer(void);

#endif // MODBUS_H