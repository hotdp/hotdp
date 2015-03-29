/*����ͷ�ļ�*/
#include <STC89C52.h>
#include <intrins.h>
/*���¶���*/
typedef unsigned char uchar;
typedef unsigned char uint;
/***************************************IO�˿ڶ���***************************************/
sbit	CE	  =P1^0;
sbit	CSN		=P1^1;
sbit	SCK	  =P1^2;
sbit 	MOSI	=P1^3;
sbit 	MISO	=P1^4;
sbit	IRQ		=P3^2;
/******************************״̬��־******************************************************/
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
uchar TX_ADDRESS[5]= {0xc2,0xc2,0xc2,0xc2,0x01};	//���ص�ַ
uchar RX_ADDRESS[5]= {0xc2,0xc2,0xc2,0xc2,0x01};	//���յ�ַ
/**************************************NRF24L01�Ĵ���ָ��*******************************************************/
#define READ_REG        0x00  	// ���Ĵ���ָ��
#define WRITE_REG       0x20 	// д�Ĵ���ָ��
#define RD_RX_PLOAD     0x61  	// ��ȡ��������ָ��
#define WR_TX_PLOAD     0xA0  	// д��������ָ��
#define FLUSH_TX        0xE1 	// ��ϴ���� FIFOָ��
#define FLUSH_RX        0xE2  	// ��ϴ���� FIFOָ��
#define REUSE_TX_PL     0xE3  	// �����ظ�װ������ָ��
#define NOP             0xFF  	// ����
/************************************SPI(nRF24L01)�Ĵ�����ַ****************************************************/
#define CONFIG          0x00  // �����շ�״̬��CRCУ��ģʽ�Լ��շ�״̬��Ӧ��ʽ
#define EN_AA           0x01  // �Զ�Ӧ��������
#define EN_RXADDR       0x02  // �����ŵ�����
#define SETUP_AW        0x03  // �շ���ַ�������
#define SETUP_RETR      0x04  // �Զ��ط���������
#define RF_CH           0x05  // ����Ƶ������
#define RF_SETUP        0x06  // �������ʡ����Ĺ�������
#define STATUS          0x07  // ״̬�Ĵ���
#define OBSERVE_TX      0x08  // ���ͼ�⹦��
#define CD              0x09  // ��ַ���           
#define RX_ADDR_P0      0x0A  // Ƶ��0�������ݵ�ַ
#define RX_ADDR_P1      0x0B  // Ƶ��1�������ݵ�ַ
#define RX_ADDR_P2      0x0C  // Ƶ��2�������ݵ�ַ
#define RX_ADDR_P3      0x0D  // Ƶ��3�������ݵ�ַ
#define RX_ADDR_P4      0x0E  // Ƶ��4�������ݵ�ַ
#define RX_ADDR_P5      0x0F  // Ƶ��5�������ݵ�ַ
#define TX_ADDR         0x10  // ���͵�ַ�Ĵ���
#define RX_PW_P0        0x11  // ����Ƶ��0�������ݳ���
#define RX_PW_P1        0x12  // ����Ƶ��0�������ݳ���
#define RX_PW_P2        0x13  // ����Ƶ��0�������ݳ���
#define RX_PW_P3        0x14  // ����Ƶ��0�������ݳ���
#define RX_PW_P4        0x15  // ����Ƶ��0�������ݳ���
#define RX_PW_P5        0x16  // ����Ƶ��0�������ݳ���
#define FIFO_STATUS     0x17  // FIFOջ��ջ��״̬�Ĵ�������
/*ȫ�ֶ���*/
uchar flag;
uchar RxBuf[32];
/***********************************************
������Delay
���ܣ���ʱ����
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
������inerDelay_us
���ܣ���ʱ����
***********************************************/
void inerDelay_us(uchar n)
{
	for(;n>0;n--)
	{
		_nop_();
	}
}
/******************************************************
������uint SPI_RW(uint uchar)
���ܣ�NRF24L01��SPIдʱ��
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
������uchar SPI_Read(uchar reg)
���ܣ�NRF24L01��SPIʱ��
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
������uint SPI_RW_Reg(uchar reg, uchar value)
���ܣ�NRF24L01��д�Ĵ�������
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
������uint SPI_Read_Buf(uchar reg, uchar *pBuf, uchar uchars)
����: ���ڶ����ݣ�reg��Ϊ�Ĵ�����ַ��pBuf��Ϊ���������ݵ�ַ��uchars���������ݵĸ���
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
������uint SPI_Write_Buf(uchar reg, uchar *pBuf, uchar uchars)
����: ����д���ݣ�Ϊ�Ĵ�����ַ��pBuf��Ϊ��д�����ݵ�ַ��uchars��д�����ݵĸ���
********************************************************/
uint SPI_Write_Buf(uchar reg, uchar *pBuf, uchar uchars)
{
	uint status,uchar_ctr;
	CSN = 0;            //SPIʹ��       
	status = SPI_RW(reg);   
	for(uchar_ctr=0; uchar_ctr<uchars; uchar_ctr++) 
	{
		SPI_RW(*pBuf++);
	}
	CSN = 1;           //�ر�SPI
	return(status);    // 
}
/****************************************************
������init_NRF24L01
���ܣ�NRF24L01��ʼ��
*************************************************/
void init_NRF24L01(void)
{
	inerDelay_us(100);
 	CE=0;    // chip enable
 	CSN=1;   // Spi disable 
 	SCK=0;   // Spi clock line init high
	SPI_Write_Buf(WRITE_REG + TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH);    // д���ص�ַ	
	SPI_Write_Buf(WRITE_REG + RX_ADDR_P0, RX_ADDRESS, RX_ADR_WIDTH); // д���ն˵�ַ
	SPI_Write_Buf(WRITE_REG + RX_ADDR_P1, 0x01, 1);                  // д���ն˵�ַ
	SPI_Write_Buf(WRITE_REG + RX_ADDR_P2, 0x02, 1);                  // д���ն˵�ַ
	SPI_Write_Buf(WRITE_REG + RX_ADDR_P3, 0x03, 1);                  // д���ն˵�ַ
	SPI_Write_Buf(WRITE_REG + RX_ADDR_P4, 0x04, 1);                  // д���ն˵�ַ
	SPI_Write_Buf(WRITE_REG + RX_ADDR_P5, 0x05, 1);                  // д���ն˵�ַ
	SPI_RW_Reg(WRITE_REG + EN_AA, 0x3f);     												 //  Ƶ��0,1,2,3,4,5�Զ�	ACKӦ������	
	SPI_RW_Reg(WRITE_REG + EN_RXADDR, 0x3f);  											 //  ������յ�ַƵ��0,1,2,3,4,5�� 
	SPI_RW_Reg(WRITE_REG + RF_CH, 0);        												 //   �����ŵ�����Ϊ2.4GHZ���շ�����һ��
	SPI_RW_Reg(WRITE_REG + RX_PW_P0, RX_PLOAD_WIDTH); //���ý������ݳ��ȣ���������Ϊ32�ֽ�
	SPI_RW_Reg(WRITE_REG + RX_PW_P1, RX_PLOAD_WIDTH); //���ý������ݳ��ȣ���������Ϊ32�ֽ�
	SPI_RW_Reg(WRITE_REG + RX_PW_P2, RX_PLOAD_WIDTH); //���ý������ݳ��ȣ���������Ϊ32�ֽ�
	SPI_RW_Reg(WRITE_REG + RX_PW_P3, RX_PLOAD_WIDTH); //���ý������ݳ��ȣ���������Ϊ32�ֽ�
	SPI_RW_Reg(WRITE_REG + RX_PW_P4, RX_PLOAD_WIDTH); //���ý������ݳ��ȣ���������Ϊ32�ֽ�
	SPI_RW_Reg(WRITE_REG + RX_PW_P5, RX_PLOAD_WIDTH); //���ý������ݳ��ȣ���������Ϊ32�ֽ�
	SPI_RW_Reg(WRITE_REG + RF_SETUP, 0x07);   		//���÷�������Ϊ1MHZ�����书��Ϊ���ֵ0dB	
//	SPI_RW_Reg(WRITE_REG + CONFIG, 0x0e);   		 // IRQ�շ�����ж���Ӧ��16λCRC��������
}
/*********************************************************
������void SetRX_Mode(void)
���ܣ����ݽ������� 
*********************************************************/
void SetRX_Mode(void)
{
	CE=0;
	SPI_RW_Reg(WRITE_REG + CONFIG, 0x0f);   		// IRQ�շ�����ж���Ӧ��16λCRC	��������
	CE = 1; 
	inerDelay_us(130);
}
/******************************************************
������unsigned char nRF24L01_RxPacket(unsigned char* rx_buf)
���ܣ����ݶ�ȡ�����rx_buf���ջ�������
*******************************************************/
uchar nRF24L01_RxPacket(uchar* rx_buf)
{
  uchar revale=0;
	CE = 0; 		                                          //SPIʹ��
	SPI_Read_Buf(RD_RX_PLOAD,rx_buf,TX_PLOAD_WIDTH);// read receive payload from RX_FIFO buffer
	revale =1;		                                      	//��ȡ������ɱ�־
	return revale;
}
/*****************************************************
������void nRF24L01_TxPacket(unsigned char * tx_buf)
���ܣ����� tx_buf������
******************************************************/
void nRF24L01_TxPacket(uchar * tx_buf)
{
	CE=0;		  	//StandBy Iģʽ	
	SPI_Write_Buf(WRITE_REG + TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH);    // д���ص�ַ	
	SPI_Write_Buf(WRITE_REG + RX_ADDR_P0, RX_ADDRESS, RX_ADR_WIDTH); // д���ն˵�ַ
	SPI_RW_Reg(WRITE_REG + CONFIG, 0x0e);   		                        // IRQ�շ�����ж���Ӧ��16λCRC��������
//	SPI_Write_Buf(WRITE_REG + RX_ADDR_P0, TX_ADDRESS, TX_ADR_WIDTH); // װ�ؽ��ն˵�ַ
	SPI_Write_Buf(WR_TX_PLOAD, tx_buf, TX_PLOAD_WIDTH); 			         // װ������	
	CE=1;		                                                           //�ø�CE���������ݷ���
	inerDelay_us(10);
}

/*****************************************************
�����SUART_Init
���ܣ����ڳ�ʼ��   //������9600
******************************************************/
void UART_Init( void )
{  							
	PCON &= 0x7f;	//�����ʲ�����
	SCON = 0x50;	//8λ����,�ɱ䲨����
	TMOD &= 0x0f;	//�����ʱ��1ģʽλ
	TMOD |= 0x20;	//�趨��ʱ��1Ϊ8λ�Զ���װ��ʽ
	TL1 = 0xFD;		//�趨��ʱ��ֵ
	TH1 = 0xFD;		//�趨��ʱ����װֵ
	ET1 = 0;		//��ֹ��ʱ��1�ж�
	TR1 = 1;		//������ʱ��1
	ES=1;
}
/*****************************************************
-�����SExter0_Init(void)
-���ܣ�
******************************************************/
void Exter0_Init(void)
{
	EA=1;
	EX0=1;
	IT0=1;
}
/*****************************************************
-�����SUART_Send(uchar dat)
-���ܣ�ͨ�����ڽ����յ����ݷ��͸�PC��
******************************************************/
void UART_Send(uchar dat)
{	
	 SBUF = dat;   
}
/*****************************************************
-�����SUART_Read(void)
-���ܣ�ͨ�����ڽ����յ����ݷ��͸�PC��
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
������main
���ܣ�������
***********************************/
void main(void)
{
	IRQ=1;
  init_NRF24L01() ;
	Exter0_Init();
	UART_Init();
	IPH=0X11;
	IP=0X10;     //�ж����ȼ��趨
	while(1);
}
/*------------------------------------
-��������:UART_ISR
--------------------------------------
-��������:�����ж�
-��ڲ���:��
-���ڲ���:��
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
		nRF24L01_TxPacket(TxBuf);	         //��Ϊ����ģʽ�����俪��ָ��     
	}
	EA=1;
}
/*------------------------------------
-��������:Exter0_isr
--------------------------------------
-��������:�ⲿ�ж�0
-��ڲ���:��
-���ڲ���:��
--------------------------------------*/
void Exter0_isr(void) interrupt 0
{
	uchar i;
	EA=0;
	sta=SPI_Read(STATUS); 
	if(RX_DR==1)
	{
		nRF24L01_RxPacket(RxBuf);    //��������
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
		SetRX_Mode();               //��Ϊ����ģʽ�������쳣ָ����뿪����
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
