#include "fan.h"

uint16_t calcFanSpeed(uint8_t percent) {
	uint16_t multiMaxSpeed = 0;

	// Умножаем проценты на максимальную скорость
	for (uint8_t i = 0; i < percent; i++) multiMaxSpeed += MAX_SPEED;

	// Делим полученное произведение на 100
	uint16_t speed = 0;
	while (multiMaxSpeed >= 100) {
		speed++;
		multiMaxSpeed -= 100;
	}

	// Округляем вверх
	if (multiMaxSpeed >= 50) speed++;
	
	if (speed > MAX_SPEED) speed = MAX_SPEED;
	return speed;
}

uint16_t initFan(uint8_t initPercent) {
	FAN_DDR |= (1 << FAN_PIN_NUM);
	// Установим TOP = 319 для 25 кГц
	ICR1 = MAX_SPEED;

	// Настроим режим Fast PWM с TOP = ICR1, неинвертирующий режим на OC1A
	// Режим Fast PWM, неинвертирующий выход OC1A, предделитель 1
	TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10); 

    // Установим начальный коэффициент заполнения, например, 75%
	uint16_t fanSpeed = calcFanSpeed(initPercent);
    OCR1A = fanSpeed;
	return fanSpeed;
}

uint16_t setFanSpeed(uint8_t percent) {
	uint8_t fanSpeed = calcFanSpeed(percent);
    OCR1A = fanSpeed;
	return fanSpeed;
}
