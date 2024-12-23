// ����������� LCD �� ���� HD44780 � ATmega16 (LM016L LCD 16x2)
// ���� http://micro-pi.ru

#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include "LCD.h"

/*
  ���������� ������� �������� ����� byte � LCD. ���� state == 0,
  �� ��������� ��� �������, ���� ���, �� ��� ������.
*/
void lcdSendNibble(char byte, char state) {
  // ���� ���������� - �� �����
  LCDCONTROLDDR |= 1<<LCD_RS | 1<<LCD_RW | 1<<LCD_E;
  // ���� ������ - �� �����
  LCDDATADDR |= 1<<LCD_D4 | 1<<LCD_D5 | 1<<LCD_D6 | 1<<LCD_D7;
  // ����� ������, RW = 0
  LCDCONTROLPORT &=  ~(1<<LCD_RW);
  // ������������� 1 � RS
  if (state) {
    // ���� ����� ������
	LCDCONTROLPORT |= 1<<LCD_RS;
  } else {
    LCDCONTROLPORT &= ~(1<<LCD_RS);
  }
  // ������� �����
  LCDCONTROLPORT |= 1<<LCD_E;
  // �������� ���� ������
  LCDDATAPORT &= ~(1<<LCD_D4 | 1<<LCD_D5 | 1<<LCD_D6 | 1<<LCD_D7);
  // ���������� �������
  if (byte & (1<<3)) {
    // �������� �����
	LCDDATAPORT |= 1<<LCD_D7;
  }
  // byte � ���� ������ ������
  if (byte & (1<<2)) {
    LCDDATAPORT |= 1<<LCD_D6;
  }

  if (byte & (1<<1)) {
    LCDDATAPORT |= 1<<LCD_D5;
  }

  if (byte & (1<<0)) {
    LCDDATAPORT |= 1<<LCD_D4;
  }
  // �����
  _delay_us(LCD_STROBEDELAY_US);
  // �������� �����. �������� ����
  LCDCONTROLPORT &= ~(1<<LCD_E);
}

/*
  ������ �������� ����� �� LCD. ���� state == 0, �� �������� �������,
  ���� ���, �� ������.
*/
char lcdGetNibble(char state) {
  char temp = 0;
  // ���� ���������� - �� �����
  LCDCONTROLDDR |=  1<<LCD_RS | 1<<LCD_RW | 1<<LCD_E;
  // ����� ������
  LCDCONTROLPORT |= 1<<LCD_RW;
  // ������������� 1 � RS
  if (state) {
    // ���� �������� ������
    LCDCONTROLPORT |=(1<<LCD_RS);
  } else {
    LCDCONTROLPORT &= ~(1<<LCD_RS);
  }
  // ������� �����
  LCDCONTROLPORT |= 1<<LCD_E;
  // ���� ������ - �� ����
  LCDDATADDR &= ~(1<<LCD_D4 | 1<<LCD_D5 | 1<<LCD_D6 | 1<<LCD_D7);
  // � ���������
  LCDDATAPORT |= 1<<LCD_D4 | 1<<LCD_D5 | 1<<LCD_D6 | 1<<LCD_D7;
  // �����
  _delay_us(LCD_STROBEDELAY_US);
  // �������� �����
  LCDCONTROLPORT &= ~(1<<LCD_E);
  // ������ ����
  if (LCDDATAPIN & (1<<LCD_D7)) {
    // �� ��������� ���������� 
    temp |= 1<<3;
  }
    
  if (LCDDATAPIN & (1<<LCD_D6)) {
    temp |= 1<<2;
  }

  if (LCDDATAPIN & (1<<LCD_D5)) {
    temp |= 1<<1;
  }
    
  if (LCDDATAPIN & (1<<LCD_D4)) {
    temp |= 1<<0;
  }
  // ���������� �����������
  return temp;
}

/*
  ������ ���� �� LCD. ���� state == 0, �� �������� �������,
  ���� ���, �� ������.
*/
char lcdRawGetByte(char state) {
  char temp = 0;
  
  temp |= lcdGetNibble(state);
  temp = temp<<4;          
  temp |= lcdGetNibble(state);
  
  return temp;
}

/*
  ��������� ���� � LCD. ���� state == 0, �� ��������� ��� �������,
  ���� ���, �� ��� ������.
*/
void lcdRawSendByte(char byte, char state) {
  lcdSendNibble((byte>>4), state);          
  lcdSendNibble(byte,state);
}

/*
  ������ ��������� LCD, ���������� 0xff, ���� ���� ��������� ����������,
  � 0x00, ���� ���.
*/
char lcdIsBusy(void) {
  /* if (lcdRawGetByte(LCD_COMMAND) & (1<<7))
    return 0xff;
  else
    return 0x00; */
  _delay_ms(DELAY);
  return 0x00;
}

/*
  ��������� ��������� ������������� �������. ������������ �����. 
*/
void lcdInit(void) {
  while (lcdIsBusy()) ;  
  lcdSendNibble(0b0010, LCD_COMMAND);
  while (lcdIsBusy()) ;
  lcdRawSendByte(0b00101000, LCD_COMMAND);
  while (lcdIsBusy()) ;
  lcdRawSendByte(0b00000001, LCD_COMMAND);
  while (lcdIsBusy()) ;
  lcdRawSendByte(0b00000110, LCD_COMMAND);
  while (lcdIsBusy()) ;
  lcdRawSendByte(0b00001100, LCD_COMMAND);
}

/*
  ������������� ����� �������: 0 - ��������, 2 - �������, 3 - �������.
  ���� �� ������ ������� LCD ��� �������� (lcdSetDisplay), �� �� ����� �������.
*/
void lcdSetCursor(char cursor) {
  while (lcdIsBusy());
  
  lcdRawSendByte((0b00001100 | cursor), LCD_COMMAND);
}

/*
  �������� ��� ��������� ����������� �������� LCD.
  ��� ������ ������ ��������� ������.
*/
void lcdSetDisplay(char state) {
  while (lcdIsBusy());
  
  lcdRawSendByte((0b00001000 | state), LCD_COMMAND);
}

/*
  ������� LCD.
*/
void lcdClear(void) {
  while (lcdIsBusy()) ;
  
  lcdRawSendByte(0b00000001, LCD_COMMAND);
}

/*
  ������������� ������ � �������� �������.
*/
void lcdGotoXY(char str, char col) {
  while (lcdIsBusy());
  
  lcdRawSendByte((0b10000000 | ((0x40 * str) + col)), LCD_COMMAND);
}

/*
  �������� ������� ����������� �� ��������� ���������� ��������
  ������ ��� �����.
*/
void lcdDisplayScroll(char pos, char dir) {
  while (pos){
    while (lcdIsBusy()) ;
  
    lcdRawSendByte((0b00011000 | dir), LCD_COMMAND);
    pos--;
  }  
}

/*
  ������� ������ �� RAM � ������� �������.
*/
void lcdPuts(char *str) {
  while (*str){
    while (lcdIsBusy()) ;
    
    lcdRawSendByte(*str++, LCD_DATA);
  }
}

/*
  ������� ������ �� flash � ������� �������.
*/
void lcdPutsf(char *str) {
  while (pgm_read_byte(str)){
    while (lcdIsBusy()) ;
    
    lcdRawSendByte(pgm_read_byte(str++), LCD_DATA);
  }
}

/*
  ������� ������ �� eeprom � ������� �������.
*/
void lcdPutse(uint8_t *str) {
  while (eeprom_read_byte(str)){
    while (lcdIsBusy()) ;
    
    lcdRawSendByte((char)(eeprom_read_byte(str++)), LCD_DATA);
  }
}

/*
  ��������� ������ � ��������������.
*/
void lcdLoadCharacter(char code, char *pattern) {
  while (lcdIsBusy());
  
  lcdRawSendByte((code<<3) | 0b01000000, LCD_COMMAND);
  
  for (char i = 0; i <= 7; i++){
    while (lcdIsBusy()) ;
        
    lcdRawSendByte(*pattern++, LCD_DATA);
  }
  while (lcdIsBusy());
  lcdRawSendByte(0b10000000, LCD_COMMAND);
}

/*
  ��������� ������ �� flash � ��������������.
*/
void lcdLoadCharacterf(char code, char *pattern) {
  while (lcdIsBusy());
  
  lcdRawSendByte((code<<3) | 0b01000000, LCD_COMMAND);
  
  for (char i = 0; i <= 7; i++){
    while (lcdIsBusy());

    lcdRawSendByte(pgm_read_byte(pattern++), LCD_DATA);
  }
  while (lcdIsBusy());
  lcdRawSendByte(0b10000000, LCD_COMMAND);
}

/*
  ��������� ������ �� eeprom � ��������������.
*/
void lcdLoadCharactere(char code, char *pattern) {
  while (lcdIsBusy());
  
  lcdRawSendByte((code<<3) | 0b01000000, LCD_COMMAND);
  
  for (char i = 0; i <= 7; i++){
    while (lcdIsBusy()) ;
        
    lcdRawSendByte(eeprom_read_byte(pattern++), LCD_DATA);
  }
  while (lcdIsBusy()) ;
  lcdRawSendByte(0b10000000, LCD_COMMAND);
}
