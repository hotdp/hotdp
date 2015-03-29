/*引用头文件*/
#include <STC89C52.h>
#include <intrins.h>
/*重新定义*/
typedef unsigned char uchar;
typedef unsigned char uint;
/***************************************IO端口定义***************************************/
sbit	CE	  =P1^0;
sbit	CSN		=P1^1;
sbit	SCK	  =P1^2;
sbit 	MOSI	=P1^3;
sbit 	MISO	=P1^4;
sbit	IRQ		=P3^2;
/******************************状态标志******************************************************/
uint 	bdata sta;   
sbit	RX_DR	=sta^6;
sbit	TX_DS	=sta^5;
sbit	MAX_RT	=sta^4;
uchar TxBuf[32]=
{
0x01,0x02,0x03,0x4,0x05,0x06,0x07,0x08,
0x09,0x10,0x11,0x12,0x13,0x14,0x15,0x16,
0x17,0x18,0x19,0x20,0x21,0x22,0x23,0x24,
0x25,0x26,0x27,0x28,0x29,0x30,0x31,0x32,
};	 
/********************************************NRF24L01*************************************/
#define TX_ADR_WIDTH    5   	// 5 uints TX address width
#define RX_ADR_WIDTH    5   	// 5 uints RX address width
#define TX_PLOAD_WIDTH  32  	// 20 uints TX payload
#define RX_PLOAD_WIDTH  32  	// 20 uints TX payload
uchar TX_ADDRESS[5]= {0xc2,0xc2,0xc2,0xc2,0x01};	//本地地址
uchar RX_ADDRESS[5]= {0xc2,0xc2,0xc2,0xc2,0x01};	//接收地址
/**************************************NRF24L01寄存器指令*******************************************************/
#define READ_REG        0x00  	// 读寄存器指令
#define WRITE_REG       0x20 	// 写寄存器指令
#define RD_RX_PLOAD     0x61  	// 读取接收数据指令
#define WR_TX_PLOAD     0xA0  	// 写待发数据指令
#define FLUSH_TX        0xE1 	// 冲洗发送 FIFO指令
#define FLUSH_RX        0xE2  	// 冲洗接收 FIFO指令
#define REUSE_TX_PL     0xE3  	// 定义重复装载数据指令
#define NOP             0xFF  	// 保留
/************************************SPI(nRF24L01)寄存器地址****************************************************/
#define CONFIG          0x00  // 配置收发状态，CRC校验模式以及收发状态响应方式
#define EN_AA           0x01  // 自动应答功能设置
#define EN_RXADDR       0x02  // 可用信道设置
#define SETUP_AW        0x03  // 收发地址宽度设置
#define SETUP_RETR      0x04  // 自动重发功能设置
#define RF_CH           0x05  // 工作频率设置
#define RF_SETUP        0x06  // 发射速率、功耗功能设置
#define STATUS          0x07  // 状态寄存器
#define OBSERVE_TX      0x08  // 发送监测功能
#define CD              0x09  // 地址检测           
#define RX_ADDR_P0      0x0A  // 频道0接收数据地址
#define RX_ADDR_P1      0x0B  // 频道1接收数据地址
#define RX_ADDR_P2      0x0C  // 频道2接收数据地址
#define RX_ADDR_P3      0x0D  // 频道3接收数据地址
#define RX_ADDR_P4      0x0E  // 频道4接收数据地址
#define RX_ADDR_P5      0x0F  // 频道5接收数据地址
#define TX_ADDR         0x10  // 发送地址寄存器
#define RX_PW_P0        0x11  // 接收频道0接收数据长度
#define RX_PW_P1        0x12  // 接收频道0接收数据长度
#define RX_PW_P2        0x13  // 接收频道0接收数据长度
#define RX_PW_P3        0x14  // 接收频道0接收数据长度
#define RX_PW_P4        0x15  // 接收频道0接收数据长度
#define RX_PW_P5        0x16  // 接收频道0接收数据长度
#define FIFO_STATUS     0x17  // FIFO栈入栈出状态寄存器设置
/*全局定义*/
uchar flag;
uchar RxBuf[32];
/***********************************************
函数：Delay
功能：延时函数
***********************************************/
void Delayms(uint ms)
{
   uchar t;
   while(ms--)
   {
   t=120;
   while(t--); 
   }
}
/***********************************************
函数：inerDelay_us
功能：延时函数
***********************************************/
void inerDelay_us(uchar n)
{
	for(;n>0;n--)
	{
		_nop_();
	}
}
/******************************************************
函数：uint SPI_RW(uint uchar)
功能：NRF24L01的SPI写时序
************************************************************/
uint SPI_RW(uint dat)
{
	uint bit_ctr;
	for(bit_ctr=0;bit_ctr<8;bit_ctr++) // output 8-bit
	{
		MOSI = (dat & 0x80);         // output 'uchar', MSB to MOSI
		dat = (dat << 1);           // shift next bit into MSB..
		SCK = 1;                      // Set SCK high..
		dat |= MISO;       		  // capture current MISO bit
		SCK = 0;            		  // ..then set SCK low again
	}
	return(dat);           		  // return read uchar
}
/***********************************************************
函数：uchar SPI_Read(uchar reg)
功能：NRF24L01的SPI时序
***********************************************************/
uchar SPI_Read(uchar reg)
{
	uchar reg_val;
	CSN = 0;                // CSN low, initialize SPI communication...
	SPI_RW(reg);            // Select register to read from..
	reg_val = SPI_RW(0);    // ..then read registervalue
	CSN = 1;                // CSN high, terminate SPI communication
	return(reg_val);        // return register value
}
/******************************************************
函数：uint SPI_RW_Reg(uchar reg, uchar value)
功能：NRF24L01读写寄存器函数
*******************************************************/
uint SPI_RW_Reg(uchar reg, uchar value)
{
	uint status;
	CSN = 0;                   // CSN low, init SPI transaction
	status = SPI_RW(reg);      // select register
	SPI_RW(value);             // ..and write value to it..
	CSN = 1;                   // CSN high again
	return(status);            // return nRF24L01 status uchar
}
/******************************************************
函数：uint SPI_Read_Buf(uchar reg, uchar *pBuf, uchar uchars)
功能: 用于读数据，reg：为寄存器地址，pBuf：为待读出数据地址，uchars：读出数据的个数
*********************************************************/
uint SPI_Read_Buf(uchar reg, uchar *pBuf, uchar uchars)
{
	uint status,uchar_ctr;
	CSN = 0;                    		// Set CSN low, init SPI tranaction
	status = SPI_RW(reg);       		// Select register to write to and read status uchar
	for(uchar_ctr=0;uchar_ctr<uchars;uchar_ctr++)
	{
		pBuf[uchar_ctr] = SPI_RW(0);    
	}
	CSN = 1;                           
	return(status);                    // return nRF24L01 status uchar
}
/*******************************************************
函数：uint SPI_Write_Buf(uchar reg, uchar *pBuf, uchar uchars)
功能: 用于写数据：为寄存器地址，pBuf：为待写入数据地址，uchars：写入数据的个数
********************************************************/
uint SPI_Write_Buf(uchar reg, uchar *pBuf, uchar uchars)
{
	uint status,uchar_ctr;
	CSN = 0;            //SPI使能       
	status = SPI_RW(reg);   
	for(uchar_ctr=0; uchar_ctr<uchars; uchar_ctr++) 
	{
		SPI_RW(*pBuf++);
	}
	CSN = 1;           //关闭SPI
	return(status);    // 
}
/****************************************************
函数：init_NRF24L01
功能：NRF24L01初始化
*************************************************/
void init_NRF24L01(void)
{
	inerDelay_us(100);
 	CE=0;    // chip enable
 	CSN=1;   // Spi disable 
 	SCK=0;   // Spi clock line init high
	SPI_Write_Buf(WRITE_REG + TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH);    // 写本地地址	
	SPI_Write_Buf(WRITE_REG + RX_ADDR_P0, RX_ADDRESS, RX_ADR_WIDTH); // 写接收端地址
	SPI_Write_Buf(WRITE_REG + RX_ADDR_P1, 0x01, 1);                  // 写接收端地址
	SPI_Write_Buf(WRITE_REG + RX_ADDR_P2, 0x02, 1);                  // 写接收端地址
	SPI_Write_Buf(WRITE_REG + RX_ADDR_P3, 0x03, 1);                  // 写接收端地址
	SPI_Write_Buf(WRITE_REG + RX_ADDR_P4, 0x04, 1);                  // 写接收端地址
	SPI_Write_Buf(WRITE_REG + RX_ADDR_P5, 0x05, 1);                  // 写接收端地址
	SPI_RW_Reg(WRITE_REG + EN_AA, 0x3f);     												 //  频道0,1,2,3,4,5自动	ACK应答允许	
	SPI_RW_Reg(WRITE_REG + EN_RXADDR, 0x3f);  											 //  允许接收地址频道0,1,2,3,4,5， 
	SPI_RW_Reg(WRITE_REG + RF_CH, 0);        												 //   设置信道工作为2.4GHZ，收发必须一致
	SPI_RW_Reg(WRITE_REG + RX_PW_P0, RX_PLOAD_WIDTH); //设置接收数据长度，本次设置为32字节
	SPI_RW_Reg(WRITE_REG + RX_PW_P1, RX_PLOAD_WIDTH); //设置接收数据长度，本次设置为32字节
	SPI_RW_Reg(WRITE_REG + RX_PW_P2, RX_PLOAD_WIDTH); //设置接收数据长度，本次设置为32字节
	SPI_RW_Reg(WRITE_REG + RX_PW_P3, RX_PLOAD_WIDTH); //设置接收数据长度，本次设置为32字节
	SPI_RW_Reg(WRITE_REG + RX_PW_P4, RX_PLOAD_WIDTH); //设置接收数据长度，本次设置为32字节
	SPI_RW_Reg(WRITE_REG + RX_PW_P5, RX_PLOAD_WIDTH); //设置接收数据长度，本次设置为32字节
	SPI_RW_Reg(WRITE_REG + RF_SETUP, 0x07);   		//设置发射速率为1MHZ，发射功率为最大值0dB	
//	SPI_RW_Reg(WRITE_REG + CONFIG, 0x0e);   		 // IRQ收发完成中断响应，16位CRC，主发送
}
/*********************************************************
函数：void SetRX_Mode(void)
功能：数据接收配置 
*********************************************************/
void SetRX_Mode(void)
{
	CE=0;
	SPI_RW_Reg(WRITE_REG + CONFIG, 0x0f);   		// IRQ收发完成中断响应，16位CRC	，主接收
	CE = 1; 
	inerDelay_us(130);
}
/******************************************************
函数：unsigned char nRF24L01_RxPacket(unsigned char* rx_buf)
功能：数据读取后放如rx_buf接收缓冲区中
*******************************************************/
uchar nRF24L01_RxPacket(uchar* rx_buf)
{
  uchar revale=0;
	CE = 0; 		                                          //SPI使能
	SPI_Read_Buf(RD_RX_PLOAD,rx_buf,TX_PLOAD_WIDTH);// read receive payload from RX_FIFO buffer
	revale =1;		                                      	//读取数据完成标志
	return revale;
}
/*****************************************************
函数：void nRF24L01_TxPacket(unsigned char * tx_buf)
功能：发送 tx_buf中数据
******************************************************/
void nRF24L01_TxPacket(uchar * tx_buf)
{
	CE=0;		  	//StandBy I模式	
	SPI_Write_Buf(WRITE_REG + TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH);    // 写本地地址	
	SPI_Write_Buf(WRITE_REG + RX_ADDR_P0, RX_ADDRESS, RX_ADR_WIDTH); // 写接收端地址
	SPI_RW_Reg(WRITE_REG + CONFIG, 0x0e);   		                        // IRQ收发完成中断响应，16位CRC，主发送
//	SPI_Write_Buf(WRITE_REG + RX_ADDR_P0, TX_ADDRESS, TX_ADR_WIDTH); // 装载接收端地址
	SPI_Write_Buf(WR_TX_PLOAD, tx_buf, TX_PLOAD_WIDTH); 			         // 装载数据	
	CE=1;		                                                           //置高CE，激发数据发送
	inerDelay_us(10);
}

/*****************************************************
函数UART_Init
功能：串口初始化   //波特率9600
******************************************************/
void UART_Init( void )
{  							
	PCON &= 0x7f;	//波特率不倍速
	SCON = 0x50;	//8位数据,可变波特率
	TMOD &= 0x0f;	//清除定时器1模式位
	TMOD |= 0x20;	//设定定时器1为8位自动重装方式
	TL1 = 0xFD;		//设定定时初值
	TH1 = 0xFD;		//设定定时器重装值
	ET1 = 0;		//禁止定时器1中断
	TR1 = 1;		//启动定时器1
	ES=1;
}
/*****************************************************
-函数Exter0_Init(void)
-功能：
******************************************************/
void Exter0_Init(void)
{
	EA=1;
	EX0=1;
	IT0=1;
}
/*****************************************************
-函数UART_Send(uchar dat)
-功能：通过串口将接收到数据发送给PC端
******************************************************/
void UART_Send(uchar dat)
{	
	 SBUF = dat;   
}
/*****************************************************
-函数UART_Read(void)
-功能：通过串口将接收到数据发送给PC端
******************************************************/
//uchar UART_Read(void)
//{
//	uchar dat;
//	while(!RI);
//	RI=0;
//	dat=SBUF;
//	return(dat);
//}
/***********************************
函数：main
功能：主函数
***********************************/
void main(void)
{
	IRQ=1;
  init_NRF24L01() ;
	Exter0_Init();
	UART_Init();
	IPH=0X11;
	IP=0X10;     //中断优先级设定
	while(1);
}
/*------------------------------------
-函数名称:UART_ISR
--------------------------------------
-函数功能:串口中断
-入口参数:无
-出口参数:无
--------------------------------------*/
void UART_isr(void) interrupt 4
{
	uchar dat;
	EA=0;
	if(TI)
	{
		TI=0;
	}
	if(RI)
	{
		RI=0;
		dat=SBUF;
	//	UART_Send(dat);
		switch(dat)
		{
			case '1':
				TX_ADDRESS[4]=0x01;
				RX_ADDRESS[4]=0x01;
				break;
			case '2':
				TX_ADDRESS[4]=0x02;
				RX_ADDRESS[4]=0x02;	
				P2=0XFD;
				break;
			case '3':
				TX_ADDRESS[4]=0x03;
				RX_ADDRESS[4]=0x03;
				P2=0XFE;
				break;
			case '4':
				TX_ADDRESS[4]=0x04;
				RX_ADDRESS[4]=0x04;
				break;
			default:break;
		}
		nRF24L01_TxPacket(TxBuf);	         //设为发射模式，发射开机指令     
	}
	EA=1;
}
/*------------------------------------
-函数名称:Exter0_isr
--------------------------------------
-函数功能:外部中断0
-入口参数:无
-出口参数:无
--------------------------------------*/
void Exter0_isr(void) interrupt 0
{
	uchar i;
	EA=0;
	sta=SPI_Read(STATUS); 
	if(RX_DR==1)
	{
		nRF24L01_RxPacket(RxBuf);    //接受数据
		for(i=0;i<32;i++)
		{
			UART_Send(RxBuf[i]);
			Delayms(600);
		}
		P0=0X00;
	}
	if(TX_DS==1)
	{
		P0=0XF0;
		SetRX_Mode();               //设为接收模式，接收异常指令和离开数据
	}
	if(MAX_RT==1)
	{
		P0=0X0F;
	}
	UART_Send(sta);
//	Delayms(200);
	IRQ=1;
	SPI_RW_Reg(WRITE_REG+STATUS,0xff);  
	EA=1;
}
