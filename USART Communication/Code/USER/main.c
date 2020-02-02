#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "lcd.h"
#include "timer.h"

//ALIENTEK 探索者STM32F407开发板 实验13
//LCD显示实验-库函数版本
//技术支持：www.openedv.com
//淘宝店铺：http://eboard.taobao.com  
//广州市星翼电子科技有限公司  
//作者：正点原子 @ALIENTEK

void LCD_display(void);

u8 FileBuff[100000];	//存放RGB565点阵格式图片的所有数据,200*240*2
u32 FileNum = 0;		//图片标志

u16 FrameByteNum = 0;		//标示当前接收到数据帧的哪个字节
//数据帧和应答帧相关定义
u8 FrameFlag = 0x7E;				//帧头标识符，固定
u8 SrcAdd = 0xff, DestAdd = 0xff;	//目标地址和原地址
u8 Command;							//命令
u8 LastFrameNum = 0;				//上一个接收到的正确的帧序号
u8 FrameNumber = 0;					//帧序号
u16 DataLength;						//数据长度
u8 FrameData[1010];					//当前帧数据缓存
u16 CheckSum = 0;
u8 CheckSumH = 0, CheckSumL = 0;	//求和校验位,高低两个字节

extern u16 POINT_COLOR;				//点阵颜色

int main(void)
{	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	delay_init(168);      			//初始化延时函数
	uart_init(115200);				//初始化串口波特率为115200
 	LCD_Init();           			//初始化LCD FSMC接口	
	TIM3_Int_Init(20-1, 8400-1);	//初始化超时定时器TIM3,时钟主频84MHz,分频8400,定时2ms
				 	
  	while(1) 
	{		 
		if (Command == 0x02)		//接收到结束帧
		{
			//LCD显示
			LCD_display();
			//delay_ms(1000);	
			//关定时器
			TIM_Cmd(TIM3, DISABLE);
			//关串口中断
			USART_Cmd(USART1, DISABLE);
			
		}     					 
	    
	} 
}

void LCD_display(void)
{
	//i:行标
	//j:列标
	u8 i, j;
	for(j = 0; j < 240; j++)
		for(i = 0; i < 200; i++)
		{
			POINT_COLOR = (((u16)FileBuff[j*400+(i+64)*2+1]) << 8) + ((u16)FileBuff[j*400+(i+64)*2]);
			LCD_DrawPoint(j, i); 
		}
}
