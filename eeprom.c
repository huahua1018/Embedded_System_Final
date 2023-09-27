#include <reg52.h> //包含標頭檔，一般情況不需要改動，標頭檔包含特殊功能寄存器的定義
#include <intrins.h>
#include "eeprom.h"

#define  _Nop()  _nop_()    //定義空指令

// 常,變數定義區                                         
sbit SDA=P3^5;            //類比I2C資料傳送位元	00111111
sbit SCL=P3^4;            //類比I2C時鐘控制位元

bit ack;
/*------------------------------------------------
                    啟動匯流排
------------------------------------------------*/
void Start_I2c()	//SDA從1變0時，SCL為1
{
  SDA=1;    //發送起始條件的資料信號
  _Nop();
  SCL=1;
  _Nop();    //起始條件建立時間大於4.7us,延時
  _Nop();
  _Nop();
  _Nop();
  _Nop();    
  SDA=0;     //發送起始信號
  _Nop();    //起始條件鎖定時間大於4μ
  _Nop();
  _Nop();
  _Nop();
  _Nop();       
  SCL=0;    //鉗住I2C匯流排，準備發送或接收資料
  _Nop();
  _Nop();
}
/*------------------------------------------------
                    結束匯流排
------------------------------------------------*/
void Stop_I2c()	//SDA從0變1時，SCL為1
{
  SDA=0;    //發送結束條件的資料信號
  _Nop();   //發送結束條件的時鐘信號
  SCL=1;    //結束條件建立時間大於4μ
  _Nop();
  _Nop();
  _Nop();
  _Nop();
  _Nop();
  SDA=1;    //發送I2C匯流排結束信號
  _Nop();
  _Nop();
  _Nop();
  _Nop();
}

/*----------------------------------------------------------------
                 位元組資料傳送函數               
函數原型: void  SendByte(unsigned char c);
功能:  將資料c發送出去,可以是位址,也可以是資料,發完後等待應答,並對
     此狀態位元進行操作.(不應答或非應答都使ack=0 假)     
     發送資料正常，ack=1; ack=0表示被控器無應答或損壞。
------------------------------------------------------------------*/
void  EEPROM_SendByte(unsigned char c)
{
	unsigned char BitCnt;
 
	for(BitCnt=0;BitCnt<8;BitCnt++)  //要傳送的資料長度為8位元
    {
     if((c<<BitCnt)&0x80)SDA=1;   //判斷發送位 &0x80=1000 0000檢查最高位元
       else  SDA=0;                
     _Nop();
     SCL=1;               //置時鐘線為高，通知被控器開始接收資料位元
     _Nop(); 
     _Nop();             //保證時鐘高電平週期大於4μ
     _Nop();
     _Nop();
     _Nop();         
     SCL=0; 
    }
    
    _Nop();
    _Nop();
    SDA=1;               //8位元發送完後釋放資料線，準備接收應答位(要接收時需設成high，因為共用同一條)
    _Nop();
    _Nop();   
    SCL=1;
    _Nop();
    _Nop();
    _Nop();
    if(SDA==1)ack=0;     
       else ack=1;        //判斷是否接收到應答信號
    SCL=0;
    _Nop();
    _Nop();
}

/*----------------------------------------------------------------
                 位元組資料傳送函數               
函數原型: unsigned char  RcvByte();
功能:  用來接收從器件傳來的資料,並判斷匯流排錯誤(不發應答信號)，
     發完後請用應答函數。  
------------------------------------------------------------------*/	
unsigned char  RcvByte()
{
  unsigned char retc;
  unsigned char BitCnt;
  
  retc=0; 
  SDA=1;             //置資料線為輸入方式
  for(BitCnt=0;BitCnt<8;BitCnt++)
  {
    _Nop();           
    SCL=0;       //置時鐘線為低，準備接收資料位元
    _Nop();
    _Nop();      //時鐘低電平週期大於4.7us
    _Nop();
    _Nop();
    _Nop();
    SCL=1;       //置時鐘線為高使資料線上資料有效
    _Nop();
    _Nop();
    retc=retc<<1;
    if(SDA==1)retc=retc+1; //讀數據位元,接收的資料位元放入retc中
    _Nop();
    _Nop(); 
  }
  SCL=0;    
  _Nop();
  _Nop();
  return(retc);
}

/*----------------------------------------------------------------
                     應答子函數
原型:  void Ack_I2c(void);
 
----------------------------------------------------------------*/
void Ack_I2c(void)
{
  
  SDA=0;     
  _Nop();
  _Nop();
  _Nop();      
  SCL=1;
  _Nop();
  _Nop();              //時鐘低電平週期大於4μ
  _Nop();
  _Nop();
  _Nop();  
  SCL=0;               //清時鐘線，鉗住I2C匯流排以便繼續接收
  _Nop();
  _Nop();    
}
/*----------------------------------------------------------------
                     非應答子函數
原型:  void NoAck_I2c(void);
 
----------------------------------------------------------------*/
void NoAck_I2c(void)
{
  
  SDA=1;
  _Nop();
  _Nop();
  _Nop();      
  SCL=1;
  _Nop();
  _Nop();              //時鐘低電平週期大於4μ
  _Nop();
  _Nop();
  _Nop();  
  SCL=0;                //清時鐘線，鉗住I2C匯流排以便繼續接收
  _Nop();
  _Nop();    
}

/*----------------------------------------------------------------
                    向有子位址器件發送多位元組資料函數               
函數原型: bit  ISendStr(unsigned char sla,unsigned char suba,ucahr *s,unsigned char no);  
功能:     從啟動匯流排到發送位址，子位址,資料，結束匯流排的全過程,從器件
          位址sla，子位址suba，發送內容是s指向的內容，發送no個位元組。
           如果返回1表示操作成功，否則操作有誤。
注意：    使用前必須已結束匯流排。
----------------------------------------------------------------*/
bit ISendStr(unsigned char sla,unsigned char suba,unsigned char *s,unsigned char no)
{
   unsigned char i;

   Start_I2c();               //啟動匯流排
   EEPROM_SendByte(sla);             //發送器件地址
     if(ack==0)return(0);
   EEPROM_SendByte(suba);            //發送器件子地址
     if(ack==0)return(0);

   for(i=0;i<no;i++)
    {   
     EEPROM_SendByte(*s);            //發送資料
       if(ack==0)return(0);
     s++;
    } 
 Stop_I2c();                  //結束匯流排
  return(1);
}

/*----------------------------------------------------------------
                    向有子位址器件讀取多位元組資料函數               
函數原型: bit  ISendStr(unsigned char sla,unsigned char suba,ucahr *s,unsigned char no);  
功能:     從啟動匯流排到發送位址，子位址,讀數據，結束匯流排的全過程,從器件
          位址sla，子位址suba，讀出的內容放入s指向的存儲區，讀no個位元組。
           如果返回1表示操作成功，否則操作有誤。
注意：    使用前必須已結束匯流排。
----------------------------------------------------------------*/
bit IRcvStr(unsigned char sla,unsigned char suba,unsigned char *s,unsigned char no)
{
   unsigned char i;

   Start_I2c();               //啟動匯流排
   EEPROM_SendByte(sla);             //發送器件地址
     if(ack==0)return(0);
   EEPROM_SendByte(suba);            //發送器件子地址
     if(ack==0)return(0);

   Start_I2c();
   EEPROM_SendByte(sla+1);			//write:最後一位0 read:最後一位1 => +1從write變read
      if(ack==0)return(0);

   for(i=0;i<no-1;i++)
    {   
     *s=RcvByte();              //發送資料
      Ack_I2c();                //發送就答位 
     s++;
    } 
   *s=RcvByte();
    NoAck_I2c();                 //發送非應位
   Stop_I2c();                    //結束匯流排
  return(1);
}