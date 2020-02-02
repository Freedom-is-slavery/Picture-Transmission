#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "lcd.h"
#include "timer.h"

//ALIENTEK ̽����STM32F407������ ʵ��13
//LCD��ʾʵ��-�⺯���汾
//����֧�֣�www.openedv.com
//�Ա����̣�http://eboard.taobao.com  
//������������ӿƼ����޹�˾  
//���ߣ�����ԭ�� @ALIENTEK

void LCD_display(void);

u8 FileBuff[100000];	//���RGB565�����ʽͼƬ����������,200*240*2
u32 FileNum = 0;		//ͼƬ��־

u16 FrameByteNum = 0;		//��ʾ��ǰ���յ�����֡���ĸ��ֽ�
//����֡��Ӧ��֡��ض���
u8 FrameFlag = 0x7E;				//֡ͷ��ʶ�����̶�
u8 SrcAdd = 0xff, DestAdd = 0xff;	//Ŀ���ַ��ԭ��ַ
u8 Command;							//����
u8 LastFrameNum = 0;				//��һ�����յ�����ȷ��֡���
u8 FrameNumber = 0;					//֡���
u16 DataLength;						//���ݳ���
u8 FrameData[1010];					//��ǰ֡���ݻ���
u16 CheckSum = 0;
u8 CheckSumH = 0, CheckSumL = 0;	//���У��λ,�ߵ������ֽ�

extern u16 POINT_COLOR;				//������ɫ

int main(void)
{	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//����ϵͳ�ж����ȼ�����2
	delay_init(168);      			//��ʼ����ʱ����
	uart_init(115200);				//��ʼ�����ڲ�����Ϊ115200
 	LCD_Init();           			//��ʼ��LCD FSMC�ӿ�	
	TIM3_Int_Init(20-1, 8400-1);	//��ʼ����ʱ��ʱ��TIM3,ʱ����Ƶ84MHz,��Ƶ8400,��ʱ2ms
				 	
  	while(1) 
	{		 
		if (Command == 0x02)		//���յ�����֡
		{
			//LCD��ʾ
			LCD_display();
			//delay_ms(1000);	
			//�ض�ʱ��
			TIM_Cmd(TIM3, DISABLE);
			//�ش����ж�
			USART_Cmd(USART1, DISABLE);
			
		}     					 
	    
	} 
}

void LCD_display(void)
{
	//i:�б�
	//j:�б�
	u8 i, j;
	for(j = 0; j < 240; j++)
		for(i = 0; i < 200; i++)
		{
			POINT_COLOR = (((u16)FileBuff[j*400+(i+64)*2+1]) << 8) + ((u16)FileBuff[j*400+(i+64)*2]);
			LCD_DrawPoint(j, i); 
		}
}
