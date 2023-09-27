#include <reg52.h> //包含標頭檔，一般情況不需要改動，標頭檔包含特殊功能寄存器的定義
#include "uart.h"

/*狀態*/
#define Error	'1'
#define Win	   	'2'
#define Loss  	'3'
#define Wrong  	'4'
#define Start	'5'
#define Time 	'6'

unsigned int com_flag = 0;	//是否收到指令
unsigned int  MAX = 4;	//result_buf最多存多少
unsigned int head = 0;	//result_buf儲存的idx
unsigned char result_buf[5]={'9','9','9','9','\n'};
unsigned char result_flag = 0;	//是否剛收到猜測結果
unsigned char command;	//儲存收到的指令

/*------------------------------------------------
                    串口初始化
------------------------------------------------*/
void InitUART  (void)
{

    SCON  = 0x50;		        // SCON: 模式 1, 8-bit UART, 使能接收  
    TMOD |= 0x20;               // TMOD: timer 1, mode 2, 8-bit 重裝
    TH1   = 0xFD;               // TH1:  重裝值 9600 串列傳輸速率 晶振 11.0592MHz  
    TR1   = 1;                  // TR1:  timer 1 打開                         
    EA    = 1;                  //打開總中斷
}

/*------------------------------------------------
                    發送一個位元組
------------------------------------------------*/
void UART_SendByte(unsigned char dat)
{
 	SBUF = dat;
 	while(!TI);
    	TI = 0;
}

/*------------------------------------------------
                    發送一個字串
------------------------------------------------*/
void UART_SendStr(unsigned char *s)
{
	while(*s!='\0')// \0 表示字串結束標誌，通過檢測是否字串末尾
  	{
		UART_SendByte(*s);
		if(*s=='\n')	//若結尾是換行符號也跳出
			break;
  		s++;
  	}
}

/*------------------------------------------------
                     串口中斷程式
------------------------------------------------*/
void UART_SER (void) interrupt 4 //串列中斷服務程式
{
   unsigned char Temp;          //定義臨時變數 
   
   if(RI)                        //判斷是接收中斷產生
   {
	  RI=0;                      //標誌位元清零
	  Temp=SBUF;                 //讀入緩衝區的值
	
	  //若接收到的是Time指令，則將遊戲時間-1
	  if(Temp==Time)
	  {
	  	game_time -= 1;
	  }
	  //若當前接收到的是猜測結果，則儲存在陣列裡
	  else if(result_flag == 1)
	  {
	  	result_buf[head]=Temp;
	  	head++;
	  }
	  //若接收到的是其他指令
	  else
	  {
	  	com_flag = 1;
		command = Temp;
		//若當前接收到的是Wrong指令，代表接下來會收到result
		if(command==Wrong)
			result_flag = 1;
	  }
	}
} 