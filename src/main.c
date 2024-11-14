#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>

#include "onewire.h"
#include "LCD.h"
#include "fan.h"
#include "USART.h"

typedef enum {
	AUTO,
	MANUAL
} mode_t;

mode_t currentMode = AUTO;
uint8_t currentSpeed = 25;

uint8_t bufferIndex = 0;
char commandBuffer[64] = {};

ISR(USART0_RX_vect) {
	uint8_t data = UDR0;

	if (data != '\r') {
		commandBuffer[bufferIndex] = data;
		bufferIndex++;
		return;
	}

	commandBuffer[bufferIndex] = '\0';
	bufferIndex = 0;
	
	char* token = strtok(commandBuffer, " ");
	
	if (strcmp(token, "set") == 0) {
		token = strtok(NULL, " ");
		
		if (token == NULL) {
			// Отсутствует аргумент

		} else if (strcmp(token, "auto") == 0) {
			// Выставлена автоматическая скорость
			currentMode = AUTO;
		} else {
			uint8_t speed = atoi(token);
			
			if (speed == 0 && token != '0') {
				// В строке не число
				
			} else {
				// Получили скорость от пользователя
				currentMode = MANUAL;
				if (speed > 100) speed = 100;
				currentSpeed = speed;
				return;
			}
		}
	} else {
		// Ввели не set
	}
		
}

// Выводит на LCD одну цифру, являющуюся результатом деления нацело dig на sub.
// Возвращает остаток этого деления
uint16_t lcd_digit(uint16_t dig, uint16_t sub) {
	char c = '0';
	while (dig >= sub) {
		dig -= sub;
		c++;
	}
	lcdRawSendByte(c, LCD_DATA);
	return dig;
}

// Выводит на LCD десятичное представление температуры с фиксированной точкой, 
// где дробная часть представлена младшими 4 разрядами
void lcd_temp(int16_t temp) {
	uint16_t unum; // число без знака
	if (temp < 0) { // отрицательное число. Отсылает знак
		unum = -temp;
		lcdRawSendByte('-', LCD_DATA); 
	} else {
		unum = temp;
	}

	uint16_t snum =  unum >> 4; // отбрасывает дробную часть
	if (snum >= 10) {
		if (snum >= 100) {
			if (snum >= 1000) {
				snum = lcd_digit(snum, 1000); // 4й разряд
			}
			snum = lcd_digit(snum, 100); // 3й разряд
		}
		snum = lcd_digit(snum, 10); // 2й разряд
	}

	lcd_digit(snum, 1); // 1й разряд
	lcdRawSendByte('.', LCD_DATA); // десятичный разделитель
	lcd_digit((((uint8_t)(unum & 0x0F)) * 10) >> 4, 1); // дробная часть
}

// Выводит температуру на LCD экран
void printTempToSreen(uint8_t msb, uint8_t lsb) {
	char msg[] = "Temp is ";
	lcdGotoXY(0, 0);
	lcdPuts(msg);
	int16_t temp = (msb << 8) | lsb;
	lcd_temp(temp);
	lcdRawSendByte('C', LCD_DATA);
}

// Отбрасывает дробную часть температуры
inline int8_t getIntTemp(uint8_t msb, uint8_t lsb) {
    return (msb << 4) | (lsb >> 4);
}

uint8_t getFanSpeedPart(uint8_t temp) {
	// Пока мокаем температуру
	return temp;
}

void printFanSpeedPartToScreen(uint8_t fanSpeedPart) {
	char msg[] = "Speed is ";
	char strFanSpeedPart[4];
	itoa(fanSpeedPart, strFanSpeedPart, 10);
	lcdGotoXY(1, 0);
	lcdPuts(msg);
	lcdPuts(strFanSpeedPart);
	lcdRawSendByte('%', LCD_DATA);
}

void printFanModeToScreen() {
	lcdGotoXY(1, 13);
	if (currentMode == AUTO) {
		char autoMode[] = "a";
		lcdPuts(autoMode);
	} else if (currentMode == MANUAL) {
		char manualMode[] = "m";
		lcdPuts(manualMode);
	}
}

void handleScratchpad(const uint8_t* scratchpad) {
	uint8_t msb = scratchpad[1];
	uint8_t lsd = scratchpad[0];

	lcdClear();

	// Выводим температуру на экран
	printTempToSreen(msb, lsd);

	// Получаем целую часть температуры
	int8_t temp = getIntTemp(msb, lsd);	
	if (temp < 0) {
		// Обработка ошибки отрицательной температуры
		
	} else {
		// Получаем проценты оборотов кулера по температуре
		// Кастуем к uint8_t так как выше проверили, что temp >= 0
		uint8_t fanSpeedPart = (currentMode == AUTO ? getFanSpeedPart((uint8_t)temp) : currentSpeed);
		setFanSpeed(fanSpeedPart);

		// Выводим проценты оборотов кулера
		printFanSpeedPartToScreen(fanSpeedPart);
		printFanModeToScreen();
	}
}

int main(void)
{
	/** 
	 * Инициализация портов для датчика температуры
	 */
	ONEWIRE_PORT &= ~_BV(ONEWIRE_PIN_NUM);
	onewire_high();

	/** 
	 * Инициализация экрана
	 */
	lcdInit();
	lcdClear();
	lcdSetDisplay(LCD_DISPLAY_ON);
	lcdSetCursor(LCD_CURSOR_OFF);

	/** 
	 * Инициализация ШИМ
	 */
	initFan(25);

	/** 
	 * Инициализация UART
	 */
	initUSART();

	sei();

	while(1)
	{
		if (onewire_skip()) { // Если у нас на шине кто-то присутствует,...
			onewire_send(0x44); // ...запускается замер температуры на всех термодатчиках
			_delay_ms(800); // Ждем измерения минимум 750 мс

			onewire_enum_init(); // Начало перечисления устройств
			for(;;) {
				uint8_t * p = onewire_enum_next(); // Очередной адрес
				if (!p) break;

				// Вывод шестнадцатиричной записи адреса в UART и рассчёт CRC
				uint8_t d = *(p++);
				uint8_t crc = 0;
				uint8_t family_code = d; // Сохранение первого байта (код семейства)
				for (uint8_t i = 0; i < 8; i++) {
					crc = onewire_crc_update(crc, d);
					d = *(p++);
				}
				
				if (crc) {
					// Не сошелся адрес
					
				} else {
					if ((family_code == 0x28) || (family_code == 0x22) || (family_code == 0x10)) { 
						// Если код семейства соответствует одному из известных... 
						// 0x10 - DS18S20, 0x28 - DS18B20, 0x22 - DS1822
						// проведём запрос scratchpad'а, считая по ходу crc
						onewire_send(0xBE); 
						uint8_t scratchpad[8];
						crc = 0;
						for (uint8_t i = 0; i < 8; i++) {
							uint8_t b = onewire_read();
							scratchpad[i] = b;
							crc = onewire_crc_update(crc, b);
						}

						if (onewire_read() != crc) {
							// Не сошелся скретчпад
							
						} else {
							// Обработка результатов измерения
							handleScratchpad(scratchpad);
						}            
					} else {
						// Неизвестное устройство
					}
				}
				// Разделитель устройств в рамках одной обработки
			}      
		} else {
			// Не найдено устройств
		}
		// Разделитель обработок
		_delay_ms(1000);
	}
}