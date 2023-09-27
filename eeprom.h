#ifndef __EEPROM_H__
#define __EEPROM_H__

void Start_I2c();
void Stop_I2c();
void  EEPROM_SendByte(unsigned char c);
unsigned char  RcvByte();
void Ack_I2c(void);
void NoAck_I2c(void);
bit ISendStr(unsigned char sla,unsigned char suba,unsigned char *s,unsigned char no);
bit IRcvStr(unsigned char sla,unsigned char suba,unsigned char *s,unsigned char no);

#endif
