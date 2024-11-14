#ifndef UART_H_
#define UART_H_

#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>

#define BAUD 9600

void initUSART();
void sendByteToUSART(uint8_t data);
uint8_t getByteFromUSART();

#endif /* UART_H_ */
