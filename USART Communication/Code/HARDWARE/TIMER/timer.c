#include "timer.h"
#include "usart.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F407开发板
//定时器 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2014/5/4
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved								
//
//最近更新时间：2019/12/3	  
////////////////////////////////////////////////////////////////////////////////// 	 


//通用定时器3中断初始化
//arr：自动重装值。
//psc：时钟预分频数
//定时器溢出时间计算方法:Tout=((arr+1)*(psc+1))/Ft us.
//Ft=定时器工作频率,单位:Mhz,这里是84MHz
//这里使用的是定时器3!
void TIM3_Int_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);  ///使能TIM3时钟
	
    TIM_TimeBaseInitStructure.TIM_Period = arr; 	//自动重装载值
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //定时器分频
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);//初始化TIM3
	
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	//TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE); //允许定时器3更新中断
	TIM_Cmd(TIM3, DISABLE); //使能定时器3,这里初始化先不使能
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn; //定时器3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x01; //抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03; //子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
}

//定时器3中断服务函数
void TIM3_IRQHandler(void)
{
	extern u8 FileBuff[100000];
	extern u32 FileNum;
	u32 i;

	extern u16 FrameByteNum;			//标示当前接收到数据帧的哪个字节
	//数据帧和应答帧相关定义
	extern u8 Command;
	extern u8 FrameNumber;				//帧序号
	extern u8 LastFrameNum;
	extern u16 DataLength;
	extern u8 FrameData[1010];			//帧缓存
	extern u16 CheckSum;
	extern u8 CheckSumH, CheckSumL;		//求和校验位,高低两个字节

	u16 ACKCheckSum;
	u8 ACKCheckSumH, ACKCheckSumL;		//应答帧校验位
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET) 		//溢出中断,表明已经接收完一帧
	{
		//TIM_ITConfig(TIM3,TIM_IT_Update,DISABLE); 	//允许定时器3更新中断
		TIM_Cmd(TIM3, DISABLE);							//停止定时器的2ms计时
		FrameByteNum = 0;
		FrameNumber = FrameData[4];

		CheckSumL = (u8)CheckSum;
		CheckSumH = (u8)(CheckSum >> 8);
		
		//判断求和校验和帧序号是否正确
		if((FrameData[DataLength + 7] == CheckSumH) && (FrameData[DataLength + 8] == CheckSumL) && \
		  ((FrameNumber == LastFrameNum)||(FrameNumber == LastFrameNum + 1)))  
		{
			//将数据存到FileBuff中
			for(i = FileNum; i < FileNum + DataLength; i++)
			{
				FileBuff[i] = FrameData[i-FileNum+7];
			}
			FileNum += DataLength;
			LastFrameNum++;

			//应答帧校验
			ACKCheckSum = 0x7E + 0xff + 0xff + Command + FrameNumber;
			ACKCheckSumL = (u8)ACKCheckSum;
			ACKCheckSumH = (u8)(ACKCheckSum >> 8);
			//发送应答帧
			USART_SendData(USART1, 0x7E);
			while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);		//等待发送完成
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
		//清除校验位、长度标示位
		CheckSumH = 0;
		CheckSumL = 0;
		CheckSum = 0;
		DataLength = 0;

	}
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //清除中断标志位
}
