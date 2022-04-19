/*
 * uart.h
 *
 *  Created on: Jun 1, 2018
 *      Author: rgrbic
 */

#ifndef UART_UART_H_
#define UART_UART_H_

#include "stm32f4xx.h"

void USART1_Init(void);
void USART_PutChar(char c);
void USART_PutString(char *s);
uint16_t USART_GetChar(void);
void USART_SendUInt_16(uint16_t num);

#endif /* UART_UART_H_ */
