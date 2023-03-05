#include <STC15F2K60S2.H>
#include "iic.h"
#include "onewire.h"
#include <stdio.h>

typedef unsigned char uchar;
typedef unsigned int uint;

code uchar SMG[]=
{0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90};

sbit C4=P3^3;
sbit C3=P3^2;
sbit R1=P4^4;
sbit R3=P3^5;
sbit L1=P0^0;
sbit L2=P0^1;
sbit L3=P0^2;
//sbit FMQ=P0^7;
//sbit JDQ=P0^5;

uint temp;
uchar dat_rb2;
uint dat_v;
bit k4=0;
bit key;
uint led_c;
bit led_f=0;
uchar command=0x00;

void Send_Byte(unsigned char dat);
void Send_String(unsigned char *str);

void Delay1ms()		//@12.000MHz
{
	unsigned char i, j;

	i = 12;
	j = 169;
	do
	{
		while (--j);
	} while (--i);
}

void Delay(uchar t)
{
	uchar i;
	for(i=0;i<t;i++)
	{
		Delay1ms();
	}
}

void HC573_Select(uchar n)
{
	switch(n)
	{
		case 4:
			P2=(P2&0x1f)|0x80;
		break;
		case 5:
			P2=(P2&0x1f)|0xa0;
		break;
		case 6:
			P2=(P2&0x1f)|0xc0;
		break;
		case 7:
			P2=(P2&0x1f)|0xe0;
		break;
		case 0:
			P2=(P2&0x1f)|0x00;
		break;
	}
}

void System_Init()
{
	HC573_Select(4);
	P0=0xff;
	HC573_Select(5);
	P0=0x00;
	HC573_Select(0);
}

void ShowSMG_Bit(uchar pos,uchar dat)
{
	HC573_Select(7);
	P0=0xff;
	HC573_Select(6);
	P0=0x01<<pos-1;
	HC573_Select(7);
	P0=dat;
	HC573_Select(0);
}

void ShowSMG_None()
{
	HC573_Select(6);
	P0=0xff;
	HC573_Select(7);
	P0=0xff;
	HC573_Select(0);
}

void ShowSMG_T()
{
	ShowSMG_Bit(1,0xc1);
	Delay(1);
	ShowSMG_Bit(2,SMG[1]);
	Delay(1);
	ShowSMG_Bit(6,SMG[temp/100]);
	Delay(1);
	ShowSMG_Bit(7,SMG[(temp/10)%10]&0x7f);
	Delay(1);
	ShowSMG_Bit(8,SMG[temp%10]);
	Delay(1);
}

void ShowSMG_V()
{
	ShowSMG_Bit(1,0xc1);
	Delay(1);
	ShowSMG_Bit(2,SMG[2]);
	Delay(1);
	ShowSMG_Bit(6,SMG[dat_v/100]&0x7f);
	Delay(1);
	ShowSMG_Bit(7,SMG[(dat_v/10)%10]);
	Delay(1);
	ShowSMG_Bit(8,SMG[dat_v%10]);
	Delay(1);
}

void ShowSMG()
{
	if(key==0)
	{
		ShowSMG_T();
	}
	else if(key==1)
	{
		ShowSMG_V();
	}
	ShowSMG_None();
}

void LEDRunning()
{
	HC573_Select(0);
	if(key==0)
	{
		L1=0;
		L2=1;
	}
	else if(key==1)
	{
		L2=0;
		L1=1;
	}
	if((k4==1)&(led_f==1))
	{
		L3=0;
	}
	else
	{
		L3=1;
	}
	HC573_Select(4);
}

void HC5_Choice()
{
	HC573_Select(0);
	if(temp>=28*10)
	{
		P0=0x10;
	}
	if(dat_v>3.6*100)
	{
		P0=0x40;
	}
	else
	{
		P0=0x00;
	}
	HC573_Select(5);
}

void DS18B20_Read()
{
	uchar LSB,MSB;
	init_ds18b20();
	Write_DS18B20(0xcc);
	Write_DS18B20(0x44);
	
	init_ds18b20();
	Write_DS18B20(0xcc);
	Write_DS18B20(0xbe);
	
	LSB=Read_DS18B20();
	MSB=Read_DS18B20();
	
	temp=(MSB<<8)|LSB;
	if((temp&0xf800)==0x0000)
	{
		temp=temp>>4;
		temp*=10;
		temp=temp+(LSB&0x0f)*0.625;
	}
}

void PCFADC()
{
	IIC_Start();
	IIC_SendByte(0x90);
	IIC_WaitAck();
	IIC_SendByte(0x43);
	IIC_WaitAck();
	IIC_Stop();
	
	IIC_Start();
	IIC_SendByte(0x91);
	IIC_WaitAck();
	dat_rb2=IIC_RecByte();
	IIC_SendAck(1);
	IIC_Stop();
}

void V_Measure()
{
	PCFADC();
	dat_v=dat_rb2*1.961;
}
void Scan_Keys()
{
	C3=0; //S5
	C4=1;
	R1=R3=1;
	if(R1==0)
	{
		Delay(20);
		if(R1==0)
		{
			while(R1==0)
			{
				ShowSMG();
				LEDRunning();
			}
			k4=0;
			TR1=1;
		}
	}
	
	C4=0;     //s4
	C3=1;
	R1=R3=1;
	if(R1==0)
	{
		Delay(20);
		if(R1==0)
		{
			while(R1==0)
			{
				ShowSMG();
				LEDRunning();
			}
			k4=1;
			TR1=0;
		}
	}
	
	C4=0;    //s12
	C3=1;
	R1=R3=1;
	if(R3==0)
	{
		Delay(20);
		if(R3==0)
		{
			while(R3==0)
			{
				ShowSMG();
				LEDRunning();
			}
			if(key==0)
			{
				Send_String("TEMP:");
				Send_String(&temp);
				Send_String("C");
//				sprintf(buffer,"TEMP:%.2fC",(int)temp);
			}
			else if(key==1)
			{
				Send_String("Voltage:");
				Send_String(&dat_v);
				Send_String("V");
			}
		}
	}
}

void Uart_Init()
{
	TMOD=0x21;
	TH1 = 0xfd;
	TL1 = 0xfd;
	TR1 = 1;
	
	SCON = 0x50;
	AUXR = 0x00;
	
	ES = 1;
	EA = 1;
}

void Send_Byte(unsigned char dat)
{
	SBUF = dat;
	while(TI == 0);
	TI = 0;
}

void Send_String(unsigned char *str)
{
	while(*str != '\0')
	{
		Send_Byte(*str++);
	}
}

void Uart_Service() interrupt 4
{
	if(RI==1)
	{
		command=SBUF;
		RI=0;
		Send_Byte(command);
	}
}

void Uart_Working()
{
	if(k4==0)
	{
		if(command != 0x00)
		{
//			switch(command & 0xf0)
//			{
//				case 0xa0:
//					key=0;
//					command = 0x00;
//				break;
//				
//				case 0xb0:
//					key=1;
//					command = 0x00;
//				break;
//			}
			switch(command)
			{
				case 0x0a:
					key=0;
					command = 0x00;
				break;
				
				case 0x0b:
					key=1;
					command = 0x00;
				break;
			}
		}
	}
}

//定时器
void Timer0_Init()
{
	TMOD=0x01;
	TH0=(65535-10000)/256;
	TL0=(65535-10000)%256;
	ET0=1;
	TR0=1;
	EA=1;
}

void Timer0_Service() interrupt 1
{
	TH0=(65535-10000)/256;
	TL0=(65535-10000)%256;
	led_c++;
	if(led_c>10)
	{
		led_f=~led_f;
		led_c=0;
	}
}

void main()
{
	System_Init();
	Timer0_Init();
	Uart_Init();
	while(1)
	{
		Scan_Keys();
		ShowSMG();
		LEDRunning();
		V_Measure();
		DS18B20_Read();
		HC5_Choice();
		Uart_Working();
 	}
}