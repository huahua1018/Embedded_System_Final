#include <reg52.h> //�]�t���Y�ɡA�@�뱡�p���ݭn��ʡA���Y�ɥ]�t�S��\��H�s�����w�q
#include <intrins.h>
#include "eeprom.h"

#define  _Nop()  _nop_()    //�w�q�ū��O

// �`,�ܼƩw�q��                                         
sbit SDA=P3^5;            //����I2C��ƶǰe�줸	00111111
sbit SCL=P3^4;            //����I2C��������줸

bit ack;
/*------------------------------------------------
                    �Ұʶ׬y��
------------------------------------------------*/
void Start_I2c()	//SDA�q1��0�ɡASCL��1
{
  SDA=1;    //�o�e�_�l���󪺸�ƫH��
  _Nop();
  SCL=1;
  _Nop();    //�_�l����إ߮ɶ��j��4.7us,����
  _Nop();
  _Nop();
  _Nop();
  _Nop();    
  SDA=0;     //�o�e�_�l�H��
  _Nop();    //�_�l������w�ɶ��j��4�g
  _Nop();
  _Nop();
  _Nop();
  _Nop();       
  SCL=0;    //�X��I2C�׬y�ơA�ǳƵo�e�α������
  _Nop();
  _Nop();
}
/*------------------------------------------------
                    �����׬y��
------------------------------------------------*/
void Stop_I2c()	//SDA�q0��1�ɡASCL��1
{
  SDA=0;    //�o�e�������󪺸�ƫH��
  _Nop();   //�o�e�������󪺮����H��
  SCL=1;    //��������إ߮ɶ��j��4�g
  _Nop();
  _Nop();
  _Nop();
  _Nop();
  _Nop();
  SDA=1;    //�o�eI2C�׬y�Ƶ����H��
  _Nop();
  _Nop();
  _Nop();
  _Nop();
}

/*----------------------------------------------------------------
                 �줸�ո�ƶǰe���               
��ƭ쫬: void  SendByte(unsigned char c);
�\��:  �N���c�o�e�X�h,�i�H�O��},�]�i�H�O���,�o���ᵥ������,�ù�
     �����A�줸�i��ާ@.(�������ΫD��������ack=0 ��)     
     �o�e��ƥ��`�Aack=1; ack=0��ܳQ�����L�����ηl�a�C
------------------------------------------------------------------*/
void  EEPROM_SendByte(unsigned char c)
{
	unsigned char BitCnt;
 
	for(BitCnt=0;BitCnt<8;BitCnt++)  //�n�ǰe����ƪ��׬�8�줸
    {
     if((c<<BitCnt)&0x80)SDA=1;   //�P�_�o�e�� &0x80=1000 0000�ˬd�̰��줸
       else  SDA=0;                
     _Nop();
     SCL=1;               //�m�����u�����A�q���Q�����}�l������Ʀ줸
     _Nop(); 
     _Nop();             //�O�Ү������q���g���j��4�g
     _Nop();
     _Nop();
     _Nop();         
     SCL=0; 
    }
    
    _Nop();
    _Nop();
    SDA=1;               //8�줸�o�e���������ƽu�A�ǳƱ���������(�n�����ɻݳ]��high�A�]���@�ΦP�@��)
    _Nop();
    _Nop();   
    SCL=1;
    _Nop();
    _Nop();
    _Nop();
    if(SDA==1)ack=0;     
       else ack=1;        //�P�_�O�_�����������H��
    SCL=0;
    _Nop();
    _Nop();
}

/*----------------------------------------------------------------
                 �줸�ո�ƶǰe���               
��ƭ쫬: unsigned char  RcvByte();
�\��:  �Ψӱ����q����ǨӪ����,�çP�_�׬y�ƿ��~(���o�����H��)�A
     �o����Х�������ơC  
------------------------------------------------------------------*/	
unsigned char  RcvByte()
{
  unsigned char retc;
  unsigned char BitCnt;
  
  retc=0; 
  SDA=1;             //�m��ƽu����J�覡
  for(BitCnt=0;BitCnt<8;BitCnt++)
  {
    _Nop();           
    SCL=0;       //�m�����u���C�A�ǳƱ�����Ʀ줸
    _Nop();
    _Nop();      //�����C�q���g���j��4.7us
    _Nop();
    _Nop();
    _Nop();
    SCL=1;       //�m�����u�����ϸ�ƽu�W��Ʀ���
    _Nop();
    _Nop();
    retc=retc<<1;
    if(SDA==1)retc=retc+1; //Ū�ƾڦ줸,��������Ʀ줸��Jretc��
    _Nop();
    _Nop(); 
  }
  SCL=0;    
  _Nop();
  _Nop();
  return(retc);
}

/*----------------------------------------------------------------
                     �����l���
�쫬:  void Ack_I2c(void);
 
----------------------------------------------------------------*/
void Ack_I2c(void)
{
  
  SDA=0;     
  _Nop();
  _Nop();
  _Nop();      
  SCL=1;
  _Nop();
  _Nop();              //�����C�q���g���j��4�g
  _Nop();
  _Nop();
  _Nop();  
  SCL=0;               //�M�����u�A�X��I2C�׬y�ƥH�K�~�򱵦�
  _Nop();
  _Nop();    
}
/*----------------------------------------------------------------
                     �D�����l���
�쫬:  void NoAck_I2c(void);
 
----------------------------------------------------------------*/
void NoAck_I2c(void)
{
  
  SDA=1;
  _Nop();
  _Nop();
  _Nop();      
  SCL=1;
  _Nop();
  _Nop();              //�����C�q���g���j��4�g
  _Nop();
  _Nop();
  _Nop();  
  SCL=0;                //�M�����u�A�X��I2C�׬y�ƥH�K�~�򱵦�
  _Nop();
  _Nop();    
}

/*----------------------------------------------------------------
                    �V���l��}����o�e�h�줸�ո�ƨ��               
��ƭ쫬: bit  ISendStr(unsigned char sla,unsigned char suba,ucahr *s,unsigned char no);  
�\��:     �q�Ұʶ׬y�ƨ�o�e��}�A�l��},��ơA�����׬y�ƪ����L�{,�q����
          ��}sla�A�l��}suba�A�o�e���e�Os���V�����e�A�o�eno�Ӧ줸�աC
           �p�G��^1��ܾާ@���\�A�_�h�ާ@���~�C
�`�N�G    �ϥΫe�����w�����׬y�ơC
----------------------------------------------------------------*/
bit ISendStr(unsigned char sla,unsigned char suba,unsigned char *s,unsigned char no)
{
   unsigned char i;

   Start_I2c();               //�Ұʶ׬y��
   EEPROM_SendByte(sla);             //�o�e����a�}
     if(ack==0)return(0);
   EEPROM_SendByte(suba);            //�o�e����l�a�}
     if(ack==0)return(0);

   for(i=0;i<no;i++)
    {   
     EEPROM_SendByte(*s);            //�o�e���
       if(ack==0)return(0);
     s++;
    } 
 Stop_I2c();                  //�����׬y��
  return(1);
}

/*----------------------------------------------------------------
                    �V���l��}����Ū���h�줸�ո�ƨ��               
��ƭ쫬: bit  ISendStr(unsigned char sla,unsigned char suba,ucahr *s,unsigned char no);  
�\��:     �q�Ұʶ׬y�ƨ�o�e��}�A�l��},Ū�ƾڡA�����׬y�ƪ����L�{,�q����
          ��}sla�A�l��}suba�AŪ�X�����e��Js���V���s�x�ϡAŪno�Ӧ줸�աC
           �p�G��^1��ܾާ@���\�A�_�h�ާ@���~�C
�`�N�G    �ϥΫe�����w�����׬y�ơC
----------------------------------------------------------------*/
bit IRcvStr(unsigned char sla,unsigned char suba,unsigned char *s,unsigned char no)
{
   unsigned char i;

   Start_I2c();               //�Ұʶ׬y��
   EEPROM_SendByte(sla);             //�o�e����a�}
     if(ack==0)return(0);
   EEPROM_SendByte(suba);            //�o�e����l�a�}
     if(ack==0)return(0);

   Start_I2c();
   EEPROM_SendByte(sla+1);			//write:�̫�@��0 read:�̫�@��1 => +1�qwrite��read
      if(ack==0)return(0);

   for(i=0;i<no-1;i++)
    {   
     *s=RcvByte();              //�o�e���
      Ack_I2c();                //�o�e�N���� 
     s++;
    } 
   *s=RcvByte();
    NoAck_I2c();                 //�o�e�D����
   Stop_I2c();                    //�����׬y��
  return(1);
}