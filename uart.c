#include <reg52.h> //�]�t���Y�ɡA�@�뱡�p���ݭn��ʡA���Y�ɥ]�t�S��\��H�s�����w�q
#include "uart.h"

/*���A*/
#define Error	'1'
#define Win	   	'2'
#define Loss  	'3'
#define Wrong  	'4'
#define Start	'5'
#define Time 	'6'

unsigned int com_flag = 0;	//�O�_������O
unsigned int  MAX = 4;	//result_buf�̦h�s�h��
unsigned int head = 0;	//result_buf�x�s��idx
unsigned char result_buf[5]={'9','9','9','9','\n'};
unsigned char result_flag = 0;	//�O�_�覬��q�����G
unsigned char command;	//�x�s���쪺���O

/*------------------------------------------------
                    ��f��l��
------------------------------------------------*/
void InitUART  (void)
{

    SCON  = 0x50;		        // SCON: �Ҧ� 1, 8-bit UART, �ϯ౵��  
    TMOD |= 0x20;               // TMOD: timer 1, mode 2, 8-bit ����
    TH1   = 0xFD;               // TH1:  ���˭� 9600 ��C�ǿ�t�v ���� 11.0592MHz  
    TR1   = 1;                  // TR1:  timer 1 ���}                         
    EA    = 1;                  //���}�`���_
}

/*------------------------------------------------
                    �o�e�@�Ӧ줸��
------------------------------------------------*/
void UART_SendByte(unsigned char dat)
{
 	SBUF = dat;
 	while(!TI);
    	TI = 0;
}

/*------------------------------------------------
                    �o�e�@�Ӧr��
------------------------------------------------*/
void UART_SendStr(unsigned char *s)
{
	while(*s!='\0')// \0 ��ܦr�굲���лx�A�q�L�˴��O�_�r�꥽��
  	{
		UART_SendByte(*s);
		if(*s=='\n')	//�Y�����O����Ÿ��]���X
			break;
  		s++;
  	}
}

/*------------------------------------------------
                     ��f���_�{��
------------------------------------------------*/
void UART_SER (void) interrupt 4 //��C���_�A�ȵ{��
{
   unsigned char Temp;          //�w�q�{���ܼ� 
   
   if(RI)                        //�P�_�O�������_����
   {
	  RI=0;                      //�лx�줸�M�s
	  Temp=SBUF;                 //Ū�J�w�İϪ���
	
	  //�Y�����쪺�OTime���O�A�h�N�C���ɶ�-1
	  if(Temp==Time)
	  {
	  	game_time -= 1;
	  }
	  //�Y��e�����쪺�O�q�����G�A�h�x�s�b�}�C��
	  else if(result_flag == 1)
	  {
	  	result_buf[head]=Temp;
	  	head++;
	  }
	  //�Y�����쪺�O��L���O
	  else
	  {
	  	com_flag = 1;
		command = Temp;
		//�Y��e�����쪺�OWrong���O�A�N���U�ӷ|����result
		if(command==Wrong)
			result_flag = 1;
	  }
	}
} 