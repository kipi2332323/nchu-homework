#include "main.h"

#define uchar unsigned char//简写
	
/* LED与数码管 */
idata uchar ucLed[8] = {0, 0, 0, 0, 0, 0, 0, 0};
idata uchar Seg_Pos;
idata uchar Seg_Buf[8] = {10, 10, 10, 10, 10, 10, 10, 10};
idata uchar Seg_Point[8] = {0, 0, 0, 0, 0, 0, 0, 0}; 

/* 串口数据*/
idata uchar Uart_Buf[100];
idata uchar Uart_Send[10];
idata uchar Uart_Rx_Index;
bit Uart_flag;
uchar Sys_Tick;
/* 时间*/
uchar ucRtc[3] = {11, 11, 11};//实际时钟数组
uchar ucRtc_temp[3] = {0,0,0};//临时显示时钟数组
unsigned int time_all_1s;//0-1S循环
uchar shanshuo=0xfe;//闪烁标志位，当为1，不闪烁 
unsigned int time500ms,time500ms_flag;//闪烁计时的
/* 数据 */
uchar Seg_Mode;//显示模式 0-正常显示 1-手动时间校准 2-闹种设置
uchar wei_xue;//选中键，0-5
uchar Alarm[10][3]={0},Num;//闹种,及个数
uchar beep_flag;
unsigned int time30s;//打铃计时时间
int i;

void Data_Proc() {
  if (time_all_1s % 50 == 0) {//500ms读取一次
		Read_Rtc(ucRtc);
    // 时间读取
  }
}
/* 键盘处理*/
void Key_Proc() {
  static uchar Key_Val, Key_Down, Key_Up, Key_Old;
  if (time_all_1s % 10) return;
  Key_Val = Key_Read();
  Key_Down = Key_Val & (Key_Old ^ Key_Val);
  Key_Up = ~Key_Val & (Key_Old ^ Key_Val);
  Key_Old = Key_Val;
	
	if(Key_Down==4)//按键四按下，且长按，休眠引脚拉高
	{
		P35=1;//休眠引脚拉高
	}
	else//按键四没按下
	{
		P35=0;//休眠引脚拉低，进入休眠
	}
	if(Seg_Mode!=0)
	{
		if(Key_Down>=10 && Key_Down<=19)//按下
		{
			if(wei_xue==0)ucRtc_temp[2] = (ucRtc_temp[2]/10%10)*10+Key_Down%10;//秒的个位
			if(wei_xue==1)ucRtc_temp[2] = ((Key_Down%10)<=6) ? (ucRtc_temp[2]%10)+(Key_Down%10)*10 : ucRtc_temp[2]; //秒的十位
			if(wei_xue==2)ucRtc_temp[1] = (ucRtc_temp[1]/10%10)*10+Key_Down%10;//分的个位
			if(wei_xue==3)ucRtc_temp[1] = ((Key_Down%10)<=6) ? (ucRtc_temp[1]%10)+(Key_Down%10)*10 : ucRtc_temp[2]; //分的十位
			if(wei_xue==4)ucRtc_temp[0] = ((Key_Down%10)<=4) ? (ucRtc_temp[0]/10%10)*10+Key_Down%10 : ucRtc_temp[2]; //时的十位
			if(wei_xue==5)ucRtc_temp[0] = ((Key_Down%10)<=2) ? (ucRtc_temp[0]%10)+(Key_Down%10)*10 : ucRtc_temp[2]; //时的十位
		}
	}
	switch(Key_Down)
	{
		case 7://模式切换键
			Seg_Mode=(++Seg_Mode) % 3;
			wei_xue=0;
			shanshuo = ~(0x01 << wei_xue);
			if(Seg_Mode==2){wei_xue=2;shanshuo = ~(0x04);}
		break;
		
		case 6://移位键
			if(Seg_Mode==1)
			{
			wei_xue=(++wei_xue) % 6;//位移键
			shanshuo = ~(0x01 << wei_xue);
			}
			if(Seg_Mode==2)
			{
			wei_xue = (wei_xue>=2) ? ((++wei_xue) % 6 ): 2;//位移键
			shanshuo = ~(0x01 << wei_xue);
			}
		break;
		
		case 5://确认键
			if(Seg_Mode==1)
			{
			Set_Rtc(ucRtc_temp);
			Seg_Mode=0;
			}
			if(Seg_Mode==2)
			{
			Alarm[Num][0]=ucRtc_temp[0]; //小时
			Alarm[Num][1]=ucRtc_temp[1]; //分钟
			Alarm[Num][2]=0; //秒
			Num=(++Num)%10;
			Seg_Mode=0;
			for(i=0;i<10;i++)//存入闹种
				EEPROM_Write(Alarm[i],0,3);
			}
		break;
	}	
	
}

/* 数码管处理*/
void Seg_Proc() {
  if (time_all_1s % 50) return;//50ms进入一次
	
	if(Seg_Mode==0){
	Seg_Buf[7]=ucRtc[2]%10;
	Seg_Buf[6]=ucRtc[2] /10 %10;
	Seg_Buf[5]=12;
	Seg_Buf[4]=ucRtc[1]%10;
	Seg_Buf[3]=ucRtc[1] /10 %10;	
	Seg_Buf[2]=12;
	Seg_Buf[1]=ucRtc[0]%10;
	Seg_Buf[0]=ucRtc[0] /10 %10;	
		
	ucRtc_temp[0]=ucRtc[0];
	ucRtc_temp[1]=ucRtc[1];
	ucRtc_temp[2]=ucRtc[2];
	}                     
	
	if(Seg_Mode==1){
	Seg_Buf[7] = (time500ms_flag || ((shanshuo >> 0)& 0x01)) ? (ucRtc_temp[2]%10) : 10;
	Seg_Buf[6] = (time500ms_flag || ((shanshuo >> 1)& 0x01)) ? (ucRtc_temp[2]/10%10) : 10;
	Seg_Buf[5] =12;
	Seg_Buf[4] = (time500ms_flag || ((shanshuo >> 2)& 0x01)) ? (ucRtc_temp[1]%10) : 10;
	Seg_Buf[3] = (time500ms_flag || ((shanshuo >> 3)& 0x01)) ? (ucRtc_temp[1]/10%10) : 10;
	Seg_Buf[2] =12;
	Seg_Buf[1] = (time500ms_flag || ((shanshuo >> 4)& 0x01)) ? (ucRtc_temp[0]%10) : 10;
	Seg_Buf[0] = (time500ms_flag || ((shanshuo >> 5)& 0x01)) ? (ucRtc_temp[0]/10%10) : 10;
	}
	
	if(Seg_Mode==2){
	Seg_Buf[7] = Num % 10;
	Seg_Buf[6] = Num / 10 %10;
	Seg_Buf[4] = (time500ms_flag || ((shanshuo >> 2)& 0x01)) ? (ucRtc_temp[1]%10) : 10;
	Seg_Buf[3] = (time500ms_flag || ((shanshuo >> 3)& 0x01)) ? (ucRtc_temp[1]/10%10) : 10;
	Seg_Buf[2] =12;
	Seg_Buf[1] = (time500ms_flag || ((shanshuo >> 4)& 0x01)) ? (ucRtc_temp[0]%10) : 10;
	Seg_Buf[0] = (time500ms_flag || ((shanshuo >> 5)& 0x01)) ? (ucRtc_temp[0]/10%10) : 10;
	}
}

void Led_Proc() {
		{
			for(i=0;i<10;i++){
				if(Alarm[i][0]==ucRtc[0] && Alarm[i][1]==ucRtc[1] && Alarm[i][2]==ucRtc[2])
					beep_flag=1;
			}
		}
		if(beep_flag==1)
		Beep(1);
		else
		Beep(0);	
}
void Uart_Proc() {
  if (Uart_Rx_Index == 0) return;
  if (Sys_Tick >= 10) {
    Sys_Tick = Uart_flag = 0;
		if(Uart_Buf[1]=='G'&&Uart_Buf[4]=='M'&&Uart_Buf[5]=='C'&&Uart_Buf[18]=='A')//符合帧数据
		{
			ucRtc_temp[0]=(Uart_Buf[7]-'0')*10+Uart_Buf[8]-'0'+8;//存入临时数组，再写入DS1302
			ucRtc_temp[1]=(Uart_Buf[9]-'0')*10+Uart_Buf[10]-'0';
			ucRtc_temp[2]=(Uart_Buf[11]-'0')*10+Uart_Buf[12]-'0';		
			Set_Rtc(ucRtc_temp);	
		}
    memset(Uart_Buf, 0, Uart_Rx_Index);
    Uart_Rx_Index = 0;
  }
}
void Timer0_Init(void)  // 1毫秒@12.000MHz
{
  AUXR &= 0x7F;  // 定时器时钟12T模式
  TMOD &= 0xF0;  // 设置定时器模式
  TMOD |= 0x05;
  TL0 = 0;  // 设置定时初始值
  TH0 = 0;  // 设置定时初始值
  TF0 = 0;  // 清除TF0标志
  TR0 = 1;  // 定时器0开始计时
  EA = 1;
}

void Timer1_Init(void)  // 1毫秒@12.000MHz
{
  AUXR &= 0xBF;  // 定时器时钟12T模式
  TMOD &= 0x0F;  // 设置定时器模式
  TL1 = 0x18;    // 设置定时初始值
  TH1 = 0xFC;    // 设置定时初始值
  TF1 = 0;       // 清除TF1标志
  TR1 = 1;       // 定时器1开始计时
  ET1 = 1;       // 使能定时器1中断
  EA = 1;
}

void Timer1_Isr(void) interrupt 3 {
  uchar i;
  if (++time_all_1s == 1000) {
    time_all_1s = 0;
  }
  if (Uart_flag) Sys_Tick++;
  Seg_Pos = (++Seg_Pos) % 8;
  Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], Seg_Point[Seg_Pos]);
  for (i = 0; i < 8; i++) Led_Disp(i, ucLed[i]);
	
	if(++time500ms>=500)//翻转电平
	{
		time500ms_flag=!time500ms_flag;
		time500ms=0;
	}
	
	if(beep_flag)//打铃30S
	{
		time30s++;
		if(time30s>=30000)//30S
		{
			time30s=0;
			beep_flag=0;
		}
	}
}
void Uart1_Isr(void) interrupt 4 {
  if (RI) {
    Uart_flag = 1;
    Sys_Tick = 0;
    Uart_Buf[Uart_Rx_Index] = SBUF;
    Uart_Rx_Index++;
    RI = 0;
  }
  if (Uart_Rx_Index > 100) Uart_Rx_Index = 0;//接收100个字节数据
}
void Delay750ms(void)  //@12.000MHz
{
  uchar data i, j, k;

  _nop_();
  _nop_();
  i = 35;
  j = 51;
  k = 182;
  do {
    do {
      while (--k);
    } while (--j);
  } while (--i);
}

void main() {
  System_Init();//一系列初始化
  Timer0_Init();
  Timer1_Init();
  Set_Rtc(ucRtc);
  rd_temperature();
	Uart1_Init();	
  Delay750ms();//暂停一下
	for(i=0;i<10;i++)//读取存储的闹种
	EEPROM_Read(Alarm[i],0,3);
  while (1) {
    Data_Proc();
    Key_Proc();
    Seg_Proc();
    Uart_Proc();
    Led_Proc();
  }
}