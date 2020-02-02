#include "timer.h"
#include "usart.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32F407������
//��ʱ�� ��������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2014/5/4
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2014-2024
//All rights reserved								
//
//�������ʱ�䣺2019/12/3	  
////////////////////////////////////////////////////////////////////////////////// 	 


//ͨ�ö�ʱ��3�жϳ�ʼ��
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��
//��ʱ�����ʱ����㷽��:Tout=((arr+1)*(psc+1))/Ft us.
//Ft=��ʱ������Ƶ��,��λ:Mhz,������84MHz
//����ʹ�õ��Ƕ�ʱ��3!
void TIM3_Int_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);  ///ʹ��TIM3ʱ��
	
    TIM_TimeBaseInitStructure.TIM_Period = arr; 	//�Զ���װ��ֵ
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //��ʱ����Ƶ
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //���ϼ���ģʽ
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);//��ʼ��TIM3
	
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	//TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE); //����ʱ��3�����ж�
	TIM_Cmd(TIM3, DISABLE); //ʹ�ܶ�ʱ��3,�����ʼ���Ȳ�ʹ��
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn; //��ʱ��3�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x01; //��ռ���ȼ�1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03; //�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
}

//��ʱ��3�жϷ�����
void TIM3_IRQHandler(void)
{
	extern u8 FileBuff[100000];
	extern u32 FileNum;
	u32 i;

	extern u16 FrameByteNum;			//��ʾ��ǰ���յ�����֡���ĸ��ֽ�
	//����֡��Ӧ��֡��ض���
	extern u8 Command;
	extern u8 FrameNumber;				//֡���
	extern u8 LastFrameNum;
	extern u16 DataLength;
	extern u8 FrameData[1010];			//֡����
	extern u16 CheckSum;
	extern u8 CheckSumH, CheckSumL;		//���У��λ,�ߵ������ֽ�

	u16 ACKCheckSum;
	u8 ACKCheckSumH, ACKCheckSumL;		//Ӧ��֡У��λ
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET) 		//����ж�,�����Ѿ�������һ֡
	{
		//TIM_ITConfig(TIM3,TIM_IT_Update,DISABLE); 	//����ʱ��3�����ж�
		TIM_Cmd(TIM3, DISABLE);							//ֹͣ��ʱ����2ms��ʱ
		FrameByteNum = 0;
		FrameNumber = FrameData[4];

		CheckSumL = (u8)CheckSum;
		CheckSumH = (u8)(CheckSum >> 8);
		
		//�ж����У���֡����Ƿ���ȷ
		if((FrameData[DataLength + 7] == CheckSumH) && (FrameData[DataLength + 8] == CheckSumL) && \
		  ((FrameNumber == LastFrameNum)||(FrameNumber == LastFrameNum + 1)))  
		{
			//�����ݴ浽FileBuff��
			for(i = FileNum; i < FileNum + DataLength; i++)
			{
				FileBuff[i] = FrameData[i-FileNum+7];
			}
			FileNum += DataLength;
			LastFrameNum++;

			//Ӧ��֡У��
			ACKCheckSum = 0x7E + 0xff + 0xff + Command + FrameNumber;
			ACKCheckSumL = (u8)ACKCheckSum;
			ACKCheckSumH = (u8)(ACKCheckSum >> 8);
			//����Ӧ��֡
			USART_SendData(USART1, 0x7E);
			while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);		//�ȴ��������
			USART_SendData(USART1, 0xff);
			while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);
			USART_SendData(USART1, 0xff);
			while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);
			USART_SendData(USART1, Command);
			while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);
			USART_SendData(USART1, FrameNumber);
			while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);
			USART_SendData(USART1, 0x00);
			while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);
			USART_SendData(USART1, 0x00);
			while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);
			USART_SendData(USART1, ACKCheckSumH);
			while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);
			USART_SendData(USART1, ACKCheckSumL);
			while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);

			
		}
		//���У��λ�����ȱ�ʾλ
		CheckSumH = 0;
		CheckSumL = 0;
		CheckSum = 0;
		DataLength = 0;

	}
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //����жϱ�־λ
}
