/*-----------------------------------------------
�����M�D
------------------------------------------------*/
#include<reg52.h> //�]�t���Y�ɡA�@�뱡�p���ݭn��ʡA���Y�ɥ]�t�S��\��H�s�����w�q 
#include <intrins.h>
#include "uart.h"
#include "eeprom.h"
#include "delay.h"

//���A
#define Error	'1'
#define Win	   	'2'
#define Loss  	'3'
#define Wrong  	'4'
#define Start	'5'
#define Time 	'6'
#define Login 	'7'
#define Logout '8'

#define UP		16
#define DOWN	17
#define CLEAN	18
#define ENTER	19

#define ACCOUNT 0
#define GUESS	1
#define RESULT	2
#define RECODRD 3

#define DataPort P0 //�w�q��ư� 
#define	KeyPort	P1	//�w�q��L��

sbit SPK   =P2^0;  //�w�q���ֿ�X��
sbit LATCH1=P3^7;	//�w�q��s�ϯ�� �q��s
sbit LATCH2=P3^6;	//               ����s

/*LED�O�N��Ѿl�q�����|*/
sbit LED1 = P2^7;
sbit LED2 = P2^6;
sbit LED3 = P2^5;
sbit LED4 = P2^4;
sbit LED5 = P2^3;

sbit Logout_but = P2^2;	//�n�X��

/*�C�q��ܾ������]�w*/                     
unsigned char code dofly_DuanMa[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f,0x77,0x7c,0x40,0x00}; // ��ܬq�X��0~9�BA�Bb�B-�B����
unsigned char code dofly_WeiMa[] = {0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f};                // ���O�����������ƽX���I�G,�Y��X
unsigned char TempData[8];  // �s�x��ܭȪ������ܼ�
unsigned char show_result_flag = 0;	//�{�b�O�_��ܲq�����G

unsigned char Timer0_H,Timer0_L,Time_delay;
unsigned char play_music=0;

/*��z�����]�w*/  
//����(do,re,mi...),����(�ĴX�C),�`��
code unsigned char win_music[]={    1,2,1,	2,2,1,	3,2,1,	6,2,2,	5,2,2,	6,2,2,   };
code unsigned char error_music[]={  1,0,1,	1,0,1,};
code unsigned char wrong_music[]={  6,0,2,	4,0,2,};
code unsigned char logout_music[]={  6,2,1,	4,2,1,};
code unsigned char login_music[]={  4,2,1,	6,2,1,};
code unsigned char loss_music[]={   4,0,3,	3,0,3,	2,0,3, 1,0,3,};
code unsigned char start_music[]={   1,1,1,	2,1,1,	3,1,1, 4,1,1,};
//1,2,3,4,5,6,7(do,re,mi...)
//�����W�v�� ���K�줸                     
code unsigned char FREQH[]={	
                                0xF2,0xF3,0xF5,0xF5,0xF6,0xF7,0xF8, 
                                0xF9,0xF9,0xFA,0xFA,0xFB,0xFB,0xFC,
                                0xFC,0xFC,0xFD,0xFD,0xFD,0xFD,0xFE,
                                0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFF,
                               } ;
//�����W�v�� �C�K�줸                         
code unsigned char FREQL[]={	
                                 0x42,0xC1,0x17,0xB6,0xD0,0xD1,0xB6,
                                 0x21,0xE1,0x8C,0xD8,0x68,0xE9,0x5B, 
                                 0x8F,0xEE,0x44, 0x6B,0xB4,0xF4,0x2D, 
                                 0x47,0x77,0xA2,0xB6,0xDA,0xFA,0x16,
                                };
/*�C���ѼƬ����]�w*/
//�b���A�̫�@�Ӭ��x�s��Ū���ɩ�J���A��
unsigned char myaccount[9]={'0','0','0','0','0','0','0','0',Logout};  
unsigned char state;			//�b�����A(Login/Logout)
unsigned char enter_account[8];	//�ثe�����J���b��
unsigned char enter_guess[5];	//�ثe�����J���q���Ʀr
unsigned char enter_cnt=0;		//�ثe�����J�F�X��
unsigned char display_num = 0;	//��� 0:�ɶ�|��e�q��/���G 1~4:record
unsigned char record_cnt = 0;	//�X�Ӳq������
unsigned int  game_time = 300;	
unsigned char game_start = 0;	//�C���O�_�}�l
unsigned char record_arr[4][8];	//�s��q���M���G�������}�C

//16 keyboard�������禡�ŧi
unsigned char KeyScan(void);
unsigned char KeyPro(void);

/*------------------------------------------------
 ��ܨ�ơA�Ω�ʺA���y�ƽX��
 ��J�Ѽ� FirstBit ��ܻݭn��ܪ��Ĥ@��A�p���2��ܱq�ĤT�ӼƽX�޶}�l���
 �p��J0��ܱq�Ĥ@����ܡC
 Num��ܻݭn��ܪ��줸���ơA�p�ݭn���99��줸���ƭȫh�ӭȿ�J2
------------------------------------------------*/
void Display(unsigned char FirstBit, unsigned char Num)
{
    static unsigned char i = 0;

    DataPort = 0; // �M�Ÿ�ơA���������v
    LATCH1 = 1;   // �q��s
    LATCH1 = 0;

    DataPort = dofly_WeiMa[i + FirstBit]; // ����X
    LATCH2 = 1;                           // ����s
    LATCH2 = 0;

    DataPort = TempData[i]; // ����ܸ�ơA�q�X
    LATCH1 = 1;             // �q��s
    LATCH1 = 0;

    i++;
    if (i == Num)
        i = 0;
}

/*------------------------------------------------
		�ھڪ��A�M�w��ܦb�C�q��ܾ��W�����e
------------------------------------------------*/
void display_set()
{
	unsigned char i;
	//�Y�D�C���i�椤�����A
	if(!game_start)	
	{
		//�Y���n�X���A�A�h���U���Ʀr�|��ܡA�٥����쪺�������'-'
		if(state==Logout)
		{
		 	for(i=0;i<enter_cnt;i++)
		 		TempData[i]=dofly_DuanMa[enter_account[i]-'0'];
		 	for(i=enter_cnt;i<8;i++)		
				TempData[i]=dofly_DuanMa[12];
		}
		//�Y���n�J���A�A�h��ܱb��
		else
		{
			for(i=0;i<8;i++)
		 		TempData[i]=dofly_DuanMa[myaccount[i]-'0'];
		}
	}
	//�Y���C���i�椤�A�h�ھ�display_num�ӱ�����ܪ�����
	else
	{
		//��ܮɶ� �M ��e�q��/���G
		if(display_num==0)
		{
			TempData[0]=dofly_DuanMa[game_time/100];
			TempData[1]=dofly_DuanMa[(game_time/10)%10];
			TempData[2]=dofly_DuanMa[game_time%10];
			TempData[3]=0x00;
			//�Y����q�����G�h��ܵ��G
			if(show_result_flag)
			{
				for(i=4;i<8;i++)
					TempData[i]=dofly_DuanMa[result_buf[i-4]];
			}
			//�_�h��ܿ�J���q��
			else
			{
				//�٥���J��������ܥ���
				for(i=4;i<4+enter_cnt;i++)
					TempData[i]=dofly_DuanMa[enter_guess[i-4]-'0'];
				for(;i<8;i++)
				 	TempData[i] = 0x00;
			}	
		}
		//��ܹL�h�q���M���G������
		else
		{
			for(i=0;i<8;i++)
				TempData[i]=dofly_DuanMa[record_arr[display_num-1][i]];
		}
	}
}   

/*------------------------------------------------
              �p�ɾ���l�ưƵ{��
------------------------------------------------*/
void Init_Timer0(void)
{
	TMOD |= 0x01;	  //�ϥμҦ�1�A16�줸�p�ɾ��A�ϥ�"|"�Ÿ��i�H�b�ϥΦh�ӭp�ɾ��ɤ����v�T		     
 	EA=1;            //�`���_���}
 	ET0=1;           //�p�ɾ����_���}
 	TR0=1;           //�p�ɾ��}�����}
}

/*------------------------------------------------
    �p�ɾ����_�Ƶ{��--�b���ΨӼ��񭵼֩M�C�q��ܾ�
------------------------------------------------*/
void Timer0_isr(void) interrupt 1 
{	
 	Display(0,8);   //�եμƽX�ޱ��y
	
	//�Y��e�O�b���񭵼�
	if(play_music)		
	{
		SPK=!SPK;
		//�ھڼ��񪺭��ֳ]�w����l�ӽվ�
		TH0=Timer0_H;
		TL0=Timer0_L;
	}
	else
	{
		//�_�h���s���1ms
		TH0=(65536-1000)/256;		 
 		TL0=(65536-1000)%256;
	}
}
 
/*------------------------------------------------
                �`�穵�ɨ��
 �U��1/4�`��ɶ��G
 ��4/4  125ms
 ��2/4  250ms
 ��3/4  187ms
------------------------------------------------*/
void delay(unsigned char t)
{
    unsigned char i;
	for(i=0;i<t;i++)
	    DelayMs(100);
    TR0=0;
 }

/*------------------------------------------------
				�q���B�z���
------------------------------------------------*/
void Song()
{
	TH0=Timer0_H;//��ȭp�ɾ��ɶ��A�M�w�W�v                                              
	TL0=Timer0_L;
	TR0=1;       //���}�p�ɾ�
	delay(Time_delay); //���ɩһݭn���`��(�o�q�����C��ƨ�]�w���ơA���|�Ұ�interrupt�A�o�X����)
}

/*------------------------------------------------
         �ھڤ��P���p���񤣦P���Ī��禡
------------------------------------------------*/
void play_song(unsigned char mode)
{
	unsigned char i=0,k;
	play_music=1;	//�]�w��e�b���񭵼�
	if(mode==Win)
	{
		while(i<18)
		{              
			k=win_music[i]+7*win_music[i+1]-1;
			Timer0_H=FREQH[k];
			Timer0_L=FREQL[k];
			Time_delay=win_music[i+2];
			i=i+3;
			Song();
    	} 

	}
	else if(mode==Wrong)
	{
		while(i<6)
		{              
			k=wrong_music[i]+7*wrong_music[i+1]-1;
			Timer0_H=FREQH[k];
			Timer0_L=FREQL[k];
			Time_delay=wrong_music[i+2]; 
			i=i+3;
			Song();
    	} 
	}
	else if(mode==Error)
	{
		while(i<6)
		{              
			k=error_music[i]+7*error_music[i+1]-1;
			Timer0_H=FREQH[k];
			Timer0_L=FREQL[k];
			Time_delay=error_music[i+2];   
			i=i+3;
			Song();
    	} 
	}
	else if(mode==Loss)
	{
		while(i<12)
		{             
			k=loss_music[i]+7*loss_music[i+1]-1;
			Timer0_H=FREQH[k];
			Timer0_L=FREQL[k];
			Time_delay=loss_music[i+2]; 
			i=i+3;
			Song();
    	} 
	}
	else if(mode==Start)
	{
		while(i<12)
		{        
			k=start_music[i]+7*start_music[i+1]-1;
			Timer0_H=FREQH[k];
			Timer0_L=FREQL[k];
			Time_delay=start_music[i+2]; 
			i=i+3;
			Song();
    	} 
	}
	else if(mode==Login)
	{
		while(i<6)
		{     
			k=login_music[i]+7*login_music[i+1]-1;
			Timer0_H=FREQH[k];
			Timer0_L=FREQL[k];
			Time_delay=login_music[i+2]; 
			i=i+3;
			Song();
    	} 
	}
	else	//logout
	{
		while(i<6)
		{       
			k=logout_music[i]+7*logout_music[i+1]-1;
			Timer0_H=FREQH[k];
			Timer0_L=FREQL[k];
			Time_delay=logout_music[i+2]; 
			i=i+3;
			Song();
    	} 
	}
	play_music=0;		//�������b���񭵼֪�flag
	TH0=(65536-1000)/256; //���s��� 1ms
 	TL0=(65536-1000)%256;
	TR0=1;       //���}�p�ɾ�
} 

/*------------------------------------------------
            �q���@�������@�ӿO���禡
------------------------------------------------*/
void led_close()
{
	switch(5-record_cnt)
	{
		case 0:
			LED1 = 1;
			break;
		case 1:
			LED2 = 1;
			break;
		case 2:
			LED3 = 1;
			break;
		case 3:
			LED4 = 1;
			break;
		case 4:
			LED5 = 1;
			break;

	}
}

/*------------------------------------------------
            ����/�}�ҩҦ��O���禡
------------------------------------------------*/
void led_all(unsigned char val)
{
	LED1 = val;
	LED2 = val;
	LED3 = val;
	LED4 = val;
	LED5 = val;
}

/*------------------------------------------------
          �B�z�C�������Ϊ�l�ƪ��禡
------------------------------------------------*/
void game_end_init()
{
	/*��l�ƩM�C���������Ѽ�*/
	led_all(1);	//�����Ҧ�led
	enter_cnt = 0;
	record_cnt = 0;
	game_start = 0;
	display_num = 0;
	show_result_flag = 0;
	result_flag = 0;
	game_time = 300;
	
	/*�Y��e���n�J���A�h�۰ʦ^�Ǫ��a�@�b��*/
	if(state==Login)
	{
		UART_SendStr("account\n");
		myaccount[8]='\n';
		UART_SendStr(myaccount);
	}
}  

/*------------------------------------------------
                    �D���
------------------------------------------------*/
void main (void)
{
	unsigned char i;
	unsigned char mynum;
	unsigned char ck_btn;

	Init_Timer0();
	InitUART();
	ES    = 1;                  //���}��f���_(�~�౵��)
	IRcvStr(0xae,4,&myaccount,9);  	//�եΦs�x���
	state=myaccount[8];				//�e�C�쬰�b���A�̫�@�쬰���A
	
	/*�Y���n�J���A�A�h�����ǰe���a�@�b��*/
	if(state==Login)
	{
		UART_SendStr("account\n");
		myaccount[8]='\n';
		UART_SendStr(myaccount);
		//����n�J����
		play_song(state);
		//�C�q��ܾ���ܱb��
		for(i=0;i<8;i++)	//�M��
			TempData[i]=0x00;
		for(i=0;i<8;i++)	//�x�s�w��J���ȹ������q�X
			TempData[i]=dofly_DuanMa[myaccount[i]-'0'];
	}

	while (1)                       
    {

		mynum = KeyPro();
		if(mynum != 0xff)		//�ˬd�O�_���U������
		{
			if(mynum==ENTER)	//�Y���U���Oenter
			{
				/*�Y��e���n�X���A�A�B�w���F8�Ӹ��X*/
				if(state==Logout&&enter_cnt==8)
				{
					//�]�w�����a�@���b��
					for(i=0;i<8;i++)
						myaccount[i]=enter_account[i];
					//���A���n�J
					state=Login;
					myaccount[8]=state;
					//�x�s��EEPROM
					ISendStr(0xae,4,&myaccount,9);
					myaccount[8]='\n';
					//�ǰe�b����ESP32
					UART_SendStr("account\n");
					UART_SendStr(myaccount);
					//����n�J����
					play_song(state);
				}
				/*�Y��e�w�b�C���i�椤�A�B�w���F4�Ӹ��X*/
				else if(game_start&&enter_cnt==4)
				{
					/*�ǰe�q��*/
					UART_SendStr("guess\n");
					enter_guess[4]='\n';
					UART_SendStr(enter_guess);
				}
			}
			else if(mynum==CLEAN)	//���U�M����
			{
				//enter_cnt����0�h��1
				enter_cnt=(enter_cnt?enter_cnt-1:0);
			}
			else if(mynum == UP)	//�V�W�d�ݬ���
			{
				display_num = (display_num +record_cnt)%(record_cnt+1);
			}
			else if(mynum == DOWN)	//�V�U�d�ݬ���
			{
				display_num = (display_num+1)%(record_cnt+1);
			}	
			//���U���O�Ʀr�A�B��J���O�b�����X
			else if(state==Logout&&enter_cnt<8)	
			{
				enter_account[enter_cnt++]=mynum;
			}
			//���U���O�Ʀr�A�B��J���O�q���Ʀr
			else if(game_start&&enter_cnt<4)
			{
				show_result_flag = 0;
				enter_guess[enter_cnt++]=mynum;
			}
		}
		
		if(com_flag)	//ESP32�Ǩӫ��O
		{
			switch(command)
			{
				case Error:
					//���񭵮�
					play_song(command);
					break;
				case Win:
					//���񭵮ĥB�I�s�C�������B�z�禡
					play_song(command);
					game_end_init();
					break;
				case Loss:
					//���񭵮ĥB�I�s�C�������B�z�禡
					play_song(command);
					game_end_init();
					break;
				case Wrong:
					//���񭵮�
					play_song(command);			
					break;
				case Start:
					//���񭵮ĥB�]�w�C���}�l
					play_song(command);
				 	game_start = 1;
					enter_cnt = 0;
					led_all(0);
					break;

			}
			//�B�z����]���Oflag�]��0
			com_flag = 0;
		}
		//�Y������F���㪺�q�����G
		if(result_flag&&head==4)
		{
			result_flag = 0;
			//�N���G�s��record�}�C��
			for(i=0;i<4;i++)
			{
				if(result_buf[i]=='A'||result_buf[i]=='B')
					result_buf[i] = result_buf[i]-'A'+10;
				else 
					result_buf[i] = result_buf[i]-'0';
				record_arr[record_cnt][i]=result_buf[i];
				record_arr[record_cnt][i+4]=enter_guess[i]-'0';
			}
			show_result_flag= 1;
			record_cnt+=1;
			//�����@�ӿO
			led_close();
			enter_cnt = 0;
			head = 0;
		}
		//�Y��e���n�J���A�A�B���U�n�X���s
		if(Logout_but==0&&state==Login)
		{
			ck_btn=1;
			//�ˬd�O�_�����F�@��
			for(i=0;i<100;i++)
			{
				if(Logout_but!=0)
				{
					ck_btn=0;
					break;
				}
				DelayMs(10); 
			}
			while(ck_btn==0);//�h�ݡA�Ѽu��
			//�Y�����@��A�h�n�X
			if(ck_btn)
			{	
				//�]�w���a�@�n�X
				state=Logout;
				myaccount[8]=state;
				//�g�Jeeprom
				ISendStr(0xae,4,&myaccount,9);
				//�ǰe�n�X�T����ESP32
				UART_SendStr("quit\n");
				//����n�X����
				play_song(state);
				game_end_init();
			}
		}
		display_set();
    }
}

unsigned char KeyScan(void)  //��L���y��ơA�ϥΦ�C�v�ű��y�k
{
 	unsigned char Val;
 	KeyPort=0xf0;//���|��m���A�C�|��ԧC (��X)
 	if(KeyPort!=0xf0)//��ܦ�������U	(read)(keyport��i/o)
   	{
    	DelayMs(10);  //�h�ݡA�Ѽu��
		if(KeyPort!=0xf0)
	  	{           //��ܦ�������U
    		KeyPort=0xfe; //�˴��Ĥ@�� //1111 1110
			if(KeyPort!=0xfe)
	  		{
			 	Val=KeyPort&0xf0;	//�O�d����4bit
	  	      	Val+=0x0e;
	  		  	while(KeyPort!=0xfe);	//�ˬd�O�_��
			  	DelayMs(10); //�h��
			  	while(KeyPort!=0xfe);//�T�w��
	     	  	return Val;
	        }
        	KeyPort=0xfd; //�˴��ĤG��	//1111 1101
			if(KeyPort!=0xfd)
	  		{
			  Val=KeyPort&0xf0;
	  	      Val+=0x0d;
	  		  while(KeyPort!=0xfd);
			  DelayMs(10); //�h��
			  while(KeyPort!=0xfd);
	     	  return Val;
	        }
    		KeyPort=0xfb; //�˴��ĤT��	//1111 1011
			if(KeyPort!=0xfb)
	  		{
			  Val=KeyPort&0xf0;
	  	      Val+=0x0b;
	  		  while(KeyPort!=0xfb);
			  DelayMs(10); //�h��
			  while(KeyPort!=0xfb);
	     	  return Val;
	        }
    		KeyPort=0xf7; //�˴��ĥ|��	//1111 0111
			if(KeyPort!=0xf7)
	  		{
			  Val=KeyPort&0xf0;
	  	      Val+=0x07;
	  		  while(KeyPort!=0xf7);
			  DelayMs(10); //�h��
			  while(KeyPort!=0xf7);
	     	  return Val;
	        }
     	}
	}
  	return 0xff;
}

unsigned char KeyPro(void)
{
	//���U����������ܬ۹������X��
	switch(KeyScan())
	{
		case 0x7e:return '1';break;	//0111 1110
		case 0x7d:return '4';break;	//0111 1101
		case 0x7b:return '7';break;	//0111 1011
		case 0x77:return '0';break;	//0111 0111
		case 0xbe:return '2';break;	//1011 1110
		case 0xbd:return '5';break;	
		case 0xbb:return '8';break;	
		case 0xb7:return 0xff;break;
		case 0xde:return '3';break;	
		case 0xdd:return '6';break;	
		case 0xdb:return '9';break;
		case 0xd7:return 0xff;break;
		case 0xee:return UP;break;	
		case 0xed:return DOWN;break;
		case 0xeb:return CLEAN;break;
		case 0xe7:return ENTER;break;
		default:return 0xff;break;
	}
}

