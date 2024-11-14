#include "USART.h"

void initUSART() {
	/*Set baud rate */
	const uint16_t ubrr = F_CPU/8/BAUD - 1;
	UBRR0H = (uint8_t)(ubrr>>8);
	UBRR0L = (uint8_t)ubrr;

	// Разрешение чтения и прерывания по чтению
	UCSR0B = (1 << RXCIE0) | (1 << RXEN0);

	// Одинарная скорость передачи
	UCSR0A = (1 << U2X0);

	/* Set frame format: 8data, 2stop bit */
	UCSR0C = (3 << UCSZ00);
}

void sendByteToUSART(uint8_t data) {
	/* Wait for empty transmit buffer */
	while (!( UCSR0A & (1 << UDRE0)));

	/* Put data into buffer, sends the data */
	UDR0 = data;
}

uint8_t getByteFromUSART() {
	/* Wait for data to be received */
	while (!(UCSR0A & (1 << RXC0)));
 
	/* Get and return received data from buffer */
	return UDR0;
}
