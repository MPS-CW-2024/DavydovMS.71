#ifndef FAN_H_
#define FAN_H_

#include <inttypes.h>
#include <avr/io.h>

#define FAN_PORT PORTD
#define FAN_DDR DDRD
#define FAN_PIN PIND
#define FAN_PIN_NUM PD5

#define MAX_SPEED 319

uint16_t initFan(uint8_t percent);
uint16_t setFanSpeed(uint8_t percent);

#endif /* FAN_H_ */
