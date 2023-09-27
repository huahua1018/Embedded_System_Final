#ifndef __UART_H__
#define __UART_H__

extern unsigned int com_flag;	//是否收到指令
extern unsigned int MAX;	//result_buf最多存多少
extern unsigned int head;	//result_buf儲存的idx
extern unsigned char result_flag;	//是否剛收到猜測結果
extern unsigned char result_buf[5];	//儲存收到的結果
extern unsigned char command;	//儲存收到的指令
extern unsigned int game_time;	//遊戲剩餘時間

void InitUART  (void);
void UART_SER (void);
void UART_SendByte(unsigned char dat);
void UART_SendStr(unsigned char *s);

#endif
