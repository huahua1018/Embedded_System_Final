/*-----------------------------------------------
期末專題
------------------------------------------------*/
#include<reg52.h> //包含標頭檔，一般情況不需要改動，標頭檔包含特殊功能寄存器的定義 
#include <intrins.h>
#include "uart.h"
#include "eeprom.h"
#include "delay.h"

//狀態
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

#define DataPort P0 //定義資料埠 
#define	KeyPort	P1	//定義鍵盤埠

sbit SPK   =P2^0;  //定義音樂輸出埠
sbit LATCH1=P3^7;	//定義鎖存使能埠 段鎖存
sbit LATCH2=P3^6;	//               位鎖存

/*LED燈代表剩餘猜測機會*/
sbit LED1 = P2^7;
sbit LED2 = P2^6;
sbit LED3 = P2^5;
sbit LED4 = P2^4;
sbit LED5 = P2^3;

sbit Logout_but = P2^2;	//登出鍵

/*七段顯示器相關設定*/                     
unsigned char code dofly_DuanMa[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f,0x77,0x7c,0x40,0x00}; // 顯示段碼值0~9、A、b、-、全黑
unsigned char code dofly_WeiMa[] = {0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f};                // 分別對應相應的數碼管點亮,即位碼
unsigned char TempData[8];  // 存儲顯示值的全域變數
unsigned char show_result_flag = 0;	//現在是否顯示猜測結果

unsigned char Timer0_H,Timer0_L,Time_delay;
unsigned char play_music=0;

/*喇叭相關設定*/  
//音階(do,re,mi...),音高(第幾列),節拍
code unsigned char win_music[]={    1,2,1,	2,2,1,	3,2,1,	6,2,2,	5,2,2,	6,2,2,   };
code unsigned char error_music[]={  1,0,1,	1,0,1,};
code unsigned char wrong_music[]={  6,0,2,	4,0,2,};
code unsigned char logout_music[]={  6,2,1,	4,2,1,};
code unsigned char login_music[]={  4,2,1,	6,2,1,};
code unsigned char loss_music[]={   4,0,3,	3,0,3,	2,0,3, 1,0,3,};
code unsigned char start_music[]={   1,1,1,	2,1,1,	3,1,1, 4,1,1,};
//1,2,3,4,5,6,7(do,re,mi...)
//音階頻率表 高八位元                     
code unsigned char FREQH[]={	
                                0xF2,0xF3,0xF5,0xF5,0xF6,0xF7,0xF8, 
                                0xF9,0xF9,0xFA,0xFA,0xFB,0xFB,0xFC,
                                0xFC,0xFC,0xFD,0xFD,0xFD,0xFD,0xFE,
                                0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFF,
                               } ;
//音階頻率表 低八位元                         
code unsigned char FREQL[]={	
                                 0x42,0xC1,0x17,0xB6,0xD0,0xD1,0xB6,
                                 0x21,0xE1,0x8C,0xD8,0x68,0xE9,0x5B, 
                                 0x8F,0xEE,0x44, 0x6B,0xB4,0xF4,0x2D, 
                                 0x47,0x77,0xA2,0xB6,0xDA,0xFA,0x16,
                                };
/*遊戲參數相關設定*/
//帳號，最後一個為儲存及讀取時放入狀態用
unsigned char myaccount[9]={'0','0','0','0','0','0','0','0',Logout};  
unsigned char state;			//帳號狀態(Login/Logout)
unsigned char enter_account[8];	//目前為止輸入的帳號
unsigned char enter_guess[5];	//目前為止輸入的猜測數字
unsigned char enter_cnt=0;		//目前為止輸入了幾個
unsigned char display_num = 0;	//顯示 0:時間|當前猜測/結果 1~4:record
unsigned char record_cnt = 0;	//幾個猜測紀錄
unsigned int  game_time = 300;	
unsigned char game_start = 0;	//遊戲是否開始
unsigned char record_arr[4][8];	//存放猜測和結果紀錄的陣列

//16 keyboard相關的函式宣告
unsigned char KeyScan(void);
unsigned char KeyPro(void);

/*------------------------------------------------
 顯示函數，用於動態掃描數碼管
 輸入參數 FirstBit 表示需要顯示的第一位，如賦值2表示從第三個數碼管開始顯示
 如輸入0表示從第一個顯示。
 Num表示需要顯示的位元元數，如需要顯示99兩位元元數值則該值輸入2
------------------------------------------------*/
void Display(unsigned char FirstBit, unsigned char Num)
{
    static unsigned char i = 0;

    DataPort = 0; // 清空資料，防止有交替重影
    LATCH1 = 1;   // 段鎖存
    LATCH1 = 0;

    DataPort = dofly_WeiMa[i + FirstBit]; // 取位碼
    LATCH2 = 1;                           // 位鎖存
    LATCH2 = 0;

    DataPort = TempData[i]; // 取顯示資料，段碼
    LATCH1 = 1;             // 段鎖存
    LATCH1 = 0;

    i++;
    if (i == Num)
        i = 0;
}

/*------------------------------------------------
		根據狀態決定顯示在七段顯示器上的內容
------------------------------------------------*/
void display_set()
{
	unsigned char i;
	//若非遊戲進行中的狀態
	if(!game_start)	
	{
		//若為登出狀態，則按下的數字會顯示，還未按到的部分顯示'-'
		if(state==Logout)
		{
		 	for(i=0;i<enter_cnt;i++)
		 		TempData[i]=dofly_DuanMa[enter_account[i]-'0'];
		 	for(i=enter_cnt;i<8;i++)		
				TempData[i]=dofly_DuanMa[12];
		}
		//若為登入狀態，則顯示帳號
		else
		{
			for(i=0;i<8;i++)
		 		TempData[i]=dofly_DuanMa[myaccount[i]-'0'];
		}
	}
	//若為遊戲進行中，則根據display_num來控制顯示的部分
	else
	{
		//顯示時間 和 當前猜測/結果
		if(display_num==0)
		{
			TempData[0]=dofly_DuanMa[game_time/100];
			TempData[1]=dofly_DuanMa[(game_time/10)%10];
			TempData[2]=dofly_DuanMa[game_time%10];
			TempData[3]=0x00;
			//若收到猜測結果則顯示結果
			if(show_result_flag)
			{
				for(i=4;i<8;i++)
					TempData[i]=dofly_DuanMa[result_buf[i-4]];
			}
			//否則顯示輸入的猜測
			else
			{
				//還未輸入的部分顯示全黑
				for(i=4;i<4+enter_cnt;i++)
					TempData[i]=dofly_DuanMa[enter_guess[i-4]-'0'];
				for(;i<8;i++)
				 	TempData[i] = 0x00;
			}	
		}
		//顯示過去猜測和結果的紀錄
		else
		{
			for(i=0;i<8;i++)
				TempData[i]=dofly_DuanMa[record_arr[display_num-1][i]];
		}
	}
}   

/*------------------------------------------------
              計時器初始化副程式
------------------------------------------------*/
void Init_Timer0(void)
{
	TMOD |= 0x01;	  //使用模式1，16位元計時器，使用"|"符號可以在使用多個計時器時不受影響		     
 	EA=1;            //總中斷打開
 	ET0=1;           //計時器中斷打開
 	TR0=1;           //計時器開關打開
}

/*------------------------------------------------
    計時器中斷副程式--在此用來播放音樂和七段顯示器
------------------------------------------------*/
void Timer0_isr(void) interrupt 1 
{	
 	Display(0,8);   //調用數碼管掃描
	
	//若當前是在播放音樂
	if(play_music)		
	{
		SPK=!SPK;
		//根據播放的音樂設定的拍子來調整
		TH0=Timer0_H;
		TL0=Timer0_L;
	}
	else
	{
		//否則重新賦值1ms
		TH0=(65536-1000)/256;		 
 		TL0=(65536-1000)%256;
	}
}
 
/*------------------------------------------------
                節拍延時函數
 各調1/4節拍時間：
 調4/4  125ms
 調2/4  250ms
 調3/4  187ms
------------------------------------------------*/
void delay(unsigned char t)
{
    unsigned char i;
	for(i=0;i<t;i++)
	    DelayMs(100);
    TR0=0;
 }

/*------------------------------------------------
				歌曲處理函數
------------------------------------------------*/
void Song()
{
	TH0=Timer0_H;//賦值計時器時間，決定頻率                                              
	TL0=Timer0_L;
	TR0=1;       //打開計時器
	delay(Time_delay); //延時所需要的節拍(這段期間每當數到設定的數，都會啟動interrupt，發出音階)
}

/*------------------------------------------------
         根據不同情況播放不同音效的函式
------------------------------------------------*/
void play_song(unsigned char mode)
{
	unsigned char i=0,k;
	play_music=1;	//設定當前在播放音樂
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
	play_music=0;		//取消正在播放音樂的flag
	TH0=(65536-1000)/256; //重新賦值 1ms
 	TL0=(65536-1000)%256;
	TR0=1;       //打開計時器
} 

/*------------------------------------------------
            猜錯一次關閉一個燈的函式
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
            關閉/開啟所有燈的函式
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
          處理遊戲結束及初始化的函式
------------------------------------------------*/
void game_end_init()
{
	/*初始化和遊戲相關的參數*/
	led_all(1);	//關閉所有led
	enter_cnt = 0;
	record_cnt = 0;
	game_start = 0;
	display_num = 0;
	show_result_flag = 0;
	result_flag = 0;
	game_time = 300;
	
	/*若當前為登入狀態則自動回傳玩家一帳號*/
	if(state==Login)
	{
		UART_SendStr("account\n");
		myaccount[8]='\n';
		UART_SendStr(myaccount);
	}
}  

/*------------------------------------------------
                    主函數
------------------------------------------------*/
void main (void)
{
	unsigned char i;
	unsigned char mynum;
	unsigned char ck_btn;

	Init_Timer0();
	InitUART();
	ES    = 1;                  //打開串口中斷(才能接收)
	IRcvStr(0xae,4,&myaccount,9);  	//調用存儲資料
	state=myaccount[8];				//前七位為帳號，最後一位為狀態
	
	/*若為登入狀態，則直接傳送玩家一帳號*/
	if(state==Login)
	{
		UART_SendStr("account\n");
		myaccount[8]='\n';
		UART_SendStr(myaccount);
		//播放登入音效
		play_song(state);
		//七段顯示器顯示帳號
		for(i=0;i<8;i++)	//清空
			TempData[i]=0x00;
		for(i=0;i<8;i++)	//儲存已輸入的值對應的段碼
			TempData[i]=dofly_DuanMa[myaccount[i]-'0'];
	}

	while (1)                       
    {

		mynum = KeyPro();
		if(mynum != 0xff)		//檢查是否按下有效鍵
		{
			if(mynum==ENTER)	//若按下的是enter
			{
				/*若當前為登出狀態，且已按了8個號碼*/
				if(state==Logout&&enter_cnt==8)
				{
					//設定為玩家一的帳號
					for(i=0;i<8;i++)
						myaccount[i]=enter_account[i];
					//狀態為登入
					state=Login;
					myaccount[8]=state;
					//儲存到EEPROM
					ISendStr(0xae,4,&myaccount,9);
					myaccount[8]='\n';
					//傳送帳號給ESP32
					UART_SendStr("account\n");
					UART_SendStr(myaccount);
					//播放登入音效
					play_song(state);
				}
				/*若當前已在遊戲進行中，且已按了4個號碼*/
				else if(game_start&&enter_cnt==4)
				{
					/*傳送猜測*/
					UART_SendStr("guess\n");
					enter_guess[4]='\n';
					UART_SendStr(enter_guess);
				}
			}
			else if(mynum==CLEAN)	//按下清除鍵
			{
				//enter_cnt不為0則減1
				enter_cnt=(enter_cnt?enter_cnt-1:0);
			}
			else if(mynum == UP)	//向上查看紀錄
			{
				display_num = (display_num +record_cnt)%(record_cnt+1);
			}
			else if(mynum == DOWN)	//向下查看紀錄
			{
				display_num = (display_num+1)%(record_cnt+1);
			}	
			//按下的是數字，且輸入的是帳號號碼
			else if(state==Logout&&enter_cnt<8)	
			{
				enter_account[enter_cnt++]=mynum;
			}
			//按下的是數字，且輸入的是猜測數字
			else if(game_start&&enter_cnt<4)
			{
				show_result_flag = 0;
				enter_guess[enter_cnt++]=mynum;
			}
		}
		
		if(com_flag)	//ESP32傳來指令
		{
			switch(command)
			{
				case Error:
					//播放音效
					play_song(command);
					break;
				case Win:
					//播放音效且呼叫遊戲結束處理函式
					play_song(command);
					game_end_init();
					break;
				case Loss:
					//播放音效且呼叫遊戲結束處理函式
					play_song(command);
					game_end_init();
					break;
				case Wrong:
					//播放音效
					play_song(command);			
					break;
				case Start:
					//播放音效且設定遊戲開始
					play_song(command);
				 	game_start = 1;
					enter_cnt = 0;
					led_all(0);
					break;

			}
			//處理完後設指令flag設為0
			com_flag = 0;
		}
		//若接收到了完整的猜測結果
		if(result_flag&&head==4)
		{
			result_flag = 0;
			//將結果存到record陣列中
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
			//關掉一個燈
			led_close();
			enter_cnt = 0;
			head = 0;
		}
		//若當前為登入狀態，且按下登出按鈕
		if(Logout_but==0&&state==Login)
		{
			ck_btn=1;
			//檢查是否長按了一秒
			for(i=0;i<100;i++)
			{
				if(Logout_but!=0)
				{
					ck_btn=0;
					break;
				}
				DelayMs(10); 
			}
			while(ck_btn==0);//去抖，解彈跳
			//若長按一秒，則登出
			if(ck_btn)
			{	
				//設定玩家一登出
				state=Logout;
				myaccount[8]=state;
				//寫入eeprom
				ISendStr(0xae,4,&myaccount,9);
				//傳送登出訊息給ESP32
				UART_SendStr("quit\n");
				//播放登出音效
				play_song(state);
				game_end_init();
			}
		}
		display_set();
    }
}

unsigned char KeyScan(void)  //鍵盤掃描函數，使用行列逐級掃描法
{
 	unsigned char Val;
 	KeyPort=0xf0;//高四位置高，低四位拉低 (丟出)
 	if(KeyPort!=0xf0)//表示有按鍵按下	(read)(keyport為i/o)
   	{
    	DelayMs(10);  //去抖，解彈跳
		if(KeyPort!=0xf0)
	  	{           //表示有按鍵按下
    		KeyPort=0xfe; //檢測第一行 //1111 1110
			if(KeyPort!=0xfe)
	  		{
			 	Val=KeyPort&0xf0;	//保留高的4bit
	  	      	Val+=0x0e;
	  		  	while(KeyPort!=0xfe);	//檢查是否放掉
			  	DelayMs(10); //去抖
			  	while(KeyPort!=0xfe);//確定放掉
	     	  	return Val;
	        }
        	KeyPort=0xfd; //檢測第二行	//1111 1101
			if(KeyPort!=0xfd)
	  		{
			  Val=KeyPort&0xf0;
	  	      Val+=0x0d;
	  		  while(KeyPort!=0xfd);
			  DelayMs(10); //去抖
			  while(KeyPort!=0xfd);
	     	  return Val;
	        }
    		KeyPort=0xfb; //檢測第三行	//1111 1011
			if(KeyPort!=0xfb)
	  		{
			  Val=KeyPort&0xf0;
	  	      Val+=0x0b;
	  		  while(KeyPort!=0xfb);
			  DelayMs(10); //去抖
			  while(KeyPort!=0xfb);
	     	  return Val;
	        }
    		KeyPort=0xf7; //檢測第四行	//1111 0111
			if(KeyPort!=0xf7)
	  		{
			  Val=KeyPort&0xf0;
	  	      Val+=0x07;
	  		  while(KeyPort!=0xf7);
			  DelayMs(10); //去抖
			  while(KeyPort!=0xf7);
	     	  return Val;
	        }
     	}
	}
  	return 0xff;
}

unsigned char KeyPro(void)
{
	//按下相應的鍵顯示相對應的碼值
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

