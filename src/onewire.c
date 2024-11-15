#include "onewire.h"

// ����� ������� ������ (reset), ������� �������� ������� �����������.
// ���� ������� ����������� �������, ���������� ��� ���������� � ���������� 1, ����� ���������� 0 
uint8_t onewire_reset() 
{
	onewire_low();
	_delay_us(640); // ����� 480..960 ���
	onewire_high();
	_delay_us(2); // ����� ����������� �������������� ���������, ����� ������� ������� ������� �� ����
	// ��� �� ����� 60 �� �� ��������� �������� �����������;
	for (uint8_t c = 80; c; c--) {
		if (!onewire_level()) {
			// ���� ��������� ������� �����������, ��� ��� ���������
			while (!onewire_level()) { } // ��� ����� ������� �����������
			return 1;
		}
		_delay_us(1);
	}
	return 0;
}

// ���������� ���� ���
// bit ������������ ��������, 0 - ����, ����� ������ �������� - �������
void onewire_send_bit(uint8_t bit) {
	onewire_low();
	if (bit) {
		_delay_us(5); // ������ �������, �� 1 �� 15 ��� (� ������ ������� �������������� ������)
		onewire_high();
		_delay_us(90); // �������� �� ���������� ��������� (�� ����� 60 ���)
	} else {
		_delay_us(90); // ������ ������� �� ���� �������� (�� ����� 60 ���, �� ����� 120 ���)
		onewire_high();
		_delay_us(5); // ����� �������������� �������� ������� �� ���� + 1 �� (�������)
	}
}

// ���������� ���� ����, ������ ������ ���, ������� ��� �����
// b - ������������ ��������
void onewire_send(uint8_t b) {
	for (uint8_t p = 8; p; p--) {
		onewire_send_bit(b & 1);
		b >>= 1;
	}
}

// ������ �������� ����, ������������ ������������.
// ���������� 0 - ���� ������� 0, �������� �� ���� �������� - ���� �������� �������
uint8_t onewire_read_bit() {
	onewire_low();
	_delay_us(2); // ������������ ������� ������, ������� 1 ���
	onewire_high();
	_delay_us(8); // ����� �� ������� �������������, ����� �� ����� 15 ���
	uint8_t r = onewire_level();
	_delay_us(80); // �������� �� ���������� ����-�����, ������� 60 ��� � ������ ������� ������
	return r;
}

// ������ ���� ����, ���������� �����������, ������� ��� �����, ���������� ����������� ��������
uint8_t onewire_read() {
	uint8_t r = 0;
	for (uint8_t p = 8; p; p--) {
		r >>= 1;
		if (onewire_read_bit()) r |= 0x80;
	}
	return r;
}

// ��������� �������� ���������� ����� crc ����������� ���� ��� ����� b.
// ���������� ���������� �������� ����������� �����
uint8_t onewire_crc_update(uint8_t crc, uint8_t b) {
	for (uint8_t p = 8; p; p--) {
		crc = ((crc ^ b) & 1) ? (crc >> 1) ^ 0b10001100 : (crc >> 1);
		b >>= 1;
	}
	return crc;
}

// ��������� ������������������ ������������� (reset + ������� ������� �����������)
// ���� ������� ����������� �������, ��������� ������� SKIP ROM
// ���������� 1, ���� ������� ����������� �������, 0 - ���� ���
uint8_t onewire_skip() {
	if (!onewire_reset())
		return 0;
	onewire_send(0xCC);
	return 1;
}

// ��������� ������������������ ������������� (reset + ������� ������� �����������)
// ���� ������� ����������� �������, ��������� ������� READ ROM, ����� ������ 8-�������� ��� ����������
//    � ��������� ��� � ����� �� ��������� buf, ������� � �������� �����
// ���������� 1, ���� ��� ��������, 0 - ���� ������� ����������� �� �������
uint8_t onewire_read_rom(uint8_t * buf) {
	if (!onewire_reset())
		return 0; 
	onewire_send(0x33);
	for (uint8_t p = 0; p < 8; p++) {
		*(buf++) = onewire_read();
	}
	return 1;
}

// ��������� ������������������ ������������� (reset + ������� ������� �����������)
// ���� ������� ����������� �������, ��������� ������� MATCH ROM, � ���������� 8-�������� ��� 
//   �� ��������� data (������� ���� �����)
// ���������� 1, ���� ������� ����������� �������, 0 - ���� ���
uint8_t onewire_match(uint8_t * data) {
	if (!onewire_reset())
		return 0;
	onewire_send(0x55);
	for (uint8_t p = 0; p < 8; p++) {
		onewire_send(*(data++));
	}
	return 1;
}

// ���������� ��� �������� �������������� ���������� ������
uint8_t onewire_enum[8]; // ��������� �������������� ����� 
uint8_t onewire_enum_fork_bit; // ��������� ������� ���, ��� ���� ��������������� (������� � �������)

// �������������� ��������� ������ ������� ���������
void onewire_enum_init() {
  for (uint8_t p = 0; p < 8; p++) {
    onewire_enum[p] = 0;
  }      
  onewire_enum_fork_bit = 65; // ������ �������
}

// ����������� ���������� �� ���� 1-wire � �������� ��������� �����.
// ���������� ��������� �� �����, ���������� �������������� �������� ������, ���� NULL, ���� ����� �������
uint8_t * onewire_enum_next() {
  if (!onewire_enum_fork_bit) { // ���� �� ���������� ���� ��� �� ���� �����������
    return 0; // �� ������ ������� ������ �� ���������
  }
  if (!onewire_reset()) {
    return 0;
  }  
  uint8_t bp = 8;
  uint8_t * pprev = &onewire_enum[0];
  uint8_t prev = *pprev;
  uint8_t next = 0;
  
  uint8_t p = 1;
  onewire_send(0xF0);
  uint8_t newfork = 0;
  for(;;) {
    uint8_t not0 = onewire_read_bit();
    uint8_t not1 = onewire_read_bit();
    if (!not0) { // ���� ������������ � ������� ��� ����
      if (!not1) { // �� ����� ������������� ��� 1 (�����)
        if (p < onewire_enum_fork_bit) { // ���� �� ����� �������� ������� ������������ ����, 
          if (prev & 1) {
            next |= 0x80; // �� �������� �������� ���� �� �������� �������
          } else {
            newfork = p; // ���� ����, �� �������� ����������� �����
          }          
        } else if (p == onewire_enum_fork_bit) {
          next |= 0x80; // ���� �� ���� ����� � ������� ��� ��� ������ �������� � ����, ������� 1
        } else {
          newfork = p; // ������ - ������� ���� � ���������� ����������� �����
        }        
      } // � ��������� ������ ���, ������� ���� � ������
    } else {
      if (!not1) { // ������������ �������
        next |= 0x80;
      } else { // ��� �� ����� �� ������ - ��������� ��������
        return 0;
      }
    }
    onewire_send_bit(next & 0x80);
    bp--;
    if (!bp) {
      *pprev = next;
      if (p >= 64)
        break;
      next = 0;
      pprev++;
      prev = *pprev;
      bp = 8;
    } else {
      if (p >= 64)
        break;
      prev >>= 1;
      next >>= 1;
    }
    p++;
  }
  onewire_enum_fork_bit = newfork;
  return &onewire_enum[0];
}

// ��������� ������������� (�����) �, ���� ������� ����������� �������,
// ��������� MATCH_ROM ��� ���������� ���������� ������� onewire_enum_next ������
// ���������� 0, ���� ������� ����������� �� ��� �������, 1 - � ���� ������
uint8_t onewire_match_last() {
  return onewire_match(&onewire_enum[0]);
}
