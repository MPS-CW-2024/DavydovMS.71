#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>

#include "onewire.h"
#include "LCD.h"
#include "fan.h"

// ������� �� LCD ���� �����, ���������� ����������� ������� ������ dig �� sub.
// ���������� ������� ����� �������
uint16_t lcd_digit(uint16_t dig, uint16_t sub) {
	char c = '0';
	while (dig >= sub) {
		dig -= sub;
		c++;
	}
	lcdRawSendByte(c, LCD_DATA);
	return dig;
}

// ������� �� LCD ���������� ������������� ����������� � ������������� ������, 
// ��� ������� ����� ������������ �������� 4 ���������
void lcd_temp(int16_t temp) {
	uint16_t unum; // ����� ��� �����
	if (temp < 0) { // ������������� �����. �������� ����
		unum = -temp;
		lcdRawSendByte('-', LCD_DATA); 
	} else {
		unum = temp;
	}

	uint16_t snum =  unum >> 4; // ����������� ������� �����
	if (snum >= 10) {
		if (snum >= 100) {
			if (snum >= 1000) {
				snum = lcd_digit(snum, 1000); // 4� ������
			}
			snum = lcd_digit(snum, 100); // 3� ������
		}
		snum = lcd_digit(snum, 10); // 2� ������
	}

	lcd_digit(snum, 1); // 1� ������
	lcdRawSendByte('.', LCD_DATA); // ���������� �����������
	lcd_digit((((uint8_t)(unum & 0x0F)) * 10) >> 4, 1); // ������� �����
}

// ������� ����������� �� LCD �����
void printTempToSreen(uint8_t msb, uint8_t lsb) {
	char msg[] = "Temp is ";
	lcdGotoXY(0, 0);
	lcdPuts(msg);
	int16_t temp = (msb << 8) | lsb;
	lcd_temp(temp);
	lcdRawSendByte('C', LCD_DATA);
}

// ����������� ������� ����� �����������
inline int8_t getIntTemp(uint8_t msb, uint8_t lsb) {
    return (msb << 4) | (lsb >> 4);
}

uint8_t getFanSpeedPart(uint8_t temp) {
	// ���� ������ �����������
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

void handleScratchpad(const uint8_t* scratchpad) {
	uint8_t msb = scratchpad[1];
	uint8_t lsd = scratchpad[0];

	// ������� ����������� �� �����
	printTempToSreen(msb, lsd);

	// �������� ����� ����� �����������
	int8_t temp = getIntTemp(msb, lsd);	
	if (temp < 0) {
		// ��������� ������ ������������� �����������
		
		return;
	}
	
	// �������� �������� �������� ������ �� �����������
	// ������� � uint8_t ��� ��� ���� ���������, ��� temp >= 0
	uint8_t fanSpeedPart = getFanSpeedPart((uint8_t)temp);
	setFanSpeed(fanSpeedPart);

	// ������� �������� �������� ������
	printFanSpeedPartToScreen(fanSpeedPart);
}

int main(void)
{
	/** 
	 * ������������� ������ ��� ������� �����������
	 */
	ONEWIRE_PORT &= ~_BV(ONEWIRE_PIN_NUM);
	onewire_high();

	/** 
	 * ������������� ������
	 */
	lcdInit();
	lcdClear();
	lcdSetDisplay(LCD_DISPLAY_ON);
	lcdSetCursor(LCD_CURSOR_OFF);

	/** 
	 * ������������� ���
	 */
	initFan(25);

	while(1)
	{
		if (onewire_skip()) { // ���� � ��� �� ���� ���-�� ������������,...
			onewire_send(0x44); // ...����������� ����� ����������� �� ���� �������������
			_delay_ms(800); // ���� ��������� ������� 750 ��

			onewire_enum_init(); // ������ ������������ ���������
			for(;;) {
				uint8_t * p = onewire_enum_next(); // ��������� �����
				if (!p) break;

				// ����� ����������������� ������ ������ � UART � ������� CRC
				uint8_t d = *(p++);
				uint8_t crc = 0;
				uint8_t family_code = d; // ���������� ������� ����� (��� ���������)
				for (uint8_t i = 0; i < 8; i++) {
					crc = onewire_crc_update(crc, d);
					d = *(p++);
				}
				
				if (crc) {
					// �� ������� �����
					
				} else {
					if ((family_code == 0x28) || (family_code == 0x22) || (family_code == 0x10)) { 
						// ���� ��� ��������� ������������� ������ �� ���������... 
						// 0x10 - DS18S20, 0x28 - DS18B20, 0x22 - DS1822
						// ������� ������ scratchpad'�, ������ �� ���� crc
						onewire_send(0xBE); 
						uint8_t scratchpad[8];
						crc = 0;
						for (uint8_t i = 0; i < 8; i++) {
							uint8_t b = onewire_read();
							scratchpad[i] = b;
							crc = onewire_crc_update(crc, b);
						}

						if (onewire_read() != crc) {
							// �� ������� ���������
							
						} else {
							// ��������� ����������� ���������
							handleScratchpad(scratchpad);
						}            
					} else {
						// ����������� ����������
					}
				}
				// ����������� ��������� � ������ ����� ���������
			}      
		} else {
			// �� ������� ���������
		}
		// ����������� ���������
		_delay_ms(10);
	}
}