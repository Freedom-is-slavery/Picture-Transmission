#include "udp_demo.h" 
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "lcd.h"
#include "malloc.h"
#include "stdio.h"
#include "string.h" 
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32F407������
//UDP ���Դ���	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2014/8/15
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved									  
//*******************************************************************************
//�޸���Ϣ:
//2019.12.22 Zhejiang University �����������ͨ�ſγ���Ŀ UDPͨ��
////////////////////////////////////////////////////////////////////////////////// 	   
 
//UDP�������ݻ�����
u8 udp_demo_recvbuf[UDP_DEMO_RX_BUFSIZE];

//UDP�������ݻ�����
u8 udp_demo_sendbuf[UDP_DEMO_TX_BUFSIZE];
//UDP����֡��Ӧ��֡��ض���
u8 FileBuff[100000]__attribute__((at(0x68040000)));	//�ⲿRAM�������ͼƬ�Ļ���,200*240*2�ֽ�
u32 FileNum = 0;									//ͼƬ��Ƭ��־
u16 FrameByteNum = 0;								//��ʾ��ǰ���յ�����֡���ĸ��ֽ�
//����֡��Ӧ��֡��ض���
const u8 FrameFlag = 0x7E;				//֡ͷ��ʶ�����̶�
const u8 SrcAdd = 0xff, DestAdd = 0xff;	//Ŀ���ַ��ԭ��ַ
u8 Command;							//����
u8 LastFrameNum = 0;				//��һ�����յ�����ȷ��֡���
u8 FrameNumber = 0;					//֡���
u16 DataLength;						//���ݳ���
//u8 FrameData[1010];				//��ǰ֡���ݻ���,���ﻻ��udp_demo_recvbuf
u16 CheckSum = 0;
u8 CheckSumH = 0, CheckSumL = 0;	//����֡���У��λ,�ߵ������ֽ�
u16 ACKCheckSum = 0;
u8 ACKCheckSumH = 0, ACKCheckSumL = 0;	//Ӧ��֡���У��λ,�ߵ������ֽ�

//UDP ����ȫ��״̬��Ǳ���
//bit7:û���õ�
//bit6:0,û���յ�����;1,�յ�������.
//bit5:0,û��������;1,��������.
//bit4~0:����
u8 udp_demo_flag;

void LCD_display(void);

//����Զ��IP��ַ
void udp_demo_set_remoteip(void)
{
	u8 *tbuf;
	//u16 xoff;
	//u8 key;
	LCD_Clear(WHITE);
	POINT_COLOR=RED;
	LCD_ShowString(30,30,200,16,16,"Explorer STM32F4");
	LCD_ShowString(30,50,200,16,16,"UDP Test");
	LCD_ShowString(30,70,200,16,16,"Remote IP Set");  
	// LCD_ShowString(30,90,200,16,16,"KEY0:+  KEY2:-");  
	// LCD_ShowString(30,110,200,16,16,"KEY_UP:OK");  

	tbuf=mymalloc(SRAMIN,100);	//�����ڴ�
	if(tbuf==NULL)	return;

	//ǰ����IP���ֺ�DHCP�õ���IPһ��
	//Զ��IP��ַ192.168.1.115
	lwipdev.remoteip[0]= lwipdev.ip[0];
	lwipdev.remoteip[1]= lwipdev.ip[1];
	lwipdev.remoteip[2]= lwipdev.ip[2]; 
	lwipdev.remoteip[3]= 115;

	//��ʾԶ��IP��ַ
	sprintf((char*)tbuf,"Remote IP:%d.%d.%d.%d",lwipdev.remoteip[0],\
			lwipdev.remoteip[1],lwipdev.remoteip[2],lwipdev.remoteip[3]);
	LCD_ShowString(30,150,210,16,16,tbuf); 
	myfree(SRAMIN,tbuf); 
} 

//UDP����
void udp_demo_test(void)
{
 	err_t err;
	struct udp_pcb *udppcb;  	//����һ��UDPЭ����ƿ�
	struct ip_addr rmtipaddr;  	//Զ��ip��ַ
 	
	u8 *tbuf;					//LCD��ʾ����ָ��
 	u8 key;
	u8 res=0;		
	u8 t=0; 
	u32 i; 						//FileBuff���ֽڴ洢��ѭ����־
	u16 j;						//����У��λCheckSumʱ�õ���ѭ����־

	udp_demo_set_remoteip();	//����Զ��(�����)IP��ַ
	
	//LCD UI����
	LCD_Clear(WHITE);	//����
	POINT_COLOR=RED; 	//��ɫ����
	LCD_ShowString(30,30,200,16,16,"Explorer STM32F4");
	LCD_ShowString(30,50,200,16,16,"UDP Test");
	LCD_ShowString(30,70,200,16,16,"ATOM@ALIENTEK");  
	LCD_ShowString(30,90,200,16,16,"KEY0:Send data");  
	LCD_ShowString(30,110,200,16,16,"KEY_UP:Quit");  
	tbuf=mymalloc(SRAMIN,200);	//�����ڴ�
	if(tbuf==NULL)return;		//�ڴ�����ʧ����,ֱ���˳�
	sprintf((char*)tbuf,"Local IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);//������IP
	LCD_ShowString(30,130,210,16,16,tbuf);  
	sprintf((char*)tbuf,"Remote IP:%d.%d.%d.%d",lwipdev.remoteip[0],lwipdev.remoteip[1],lwipdev.remoteip[2],lwipdev.remoteip[3]);//Զ��IP
	LCD_ShowString(30,150,210,16,16,tbuf);  
	sprintf((char*)tbuf,"Remote Port:%d",UDP_DEMO_PORT);//�ͻ��˶˿ں�
	LCD_ShowString(30,170,210,16,16,tbuf);
	POINT_COLOR=BLUE;
	LCD_ShowString(30,190,210,16,16,"STATUS:Disconnected"); 


	udppcb=udp_new();

	if(udppcb)//�����ɹ�
	{
		//��ʱres=0
		IP4_ADDR(&rmtipaddr,lwipdev.remoteip[0],lwipdev.remoteip[1],lwipdev.remoteip[2],lwipdev.remoteip[3]);
		err=udp_connect(udppcb,&rmtipaddr,UDP_DEMO_PORT);//UDP�ͻ������ӵ�ָ��IP��ַ�Ͷ˿ںŵķ�����
		if(err==ERR_OK)
		{
			err=udp_bind(udppcb,IP_ADDR_ANY,UDP_DEMO_PORT);//�󶨱���IP��ַ��˿ں�
			if(err==ERR_OK)	//�����
			{
				udp_recv(udppcb,udp_demo_recv,NULL);//ע����ջص����� 
				LCD_ShowString(30,190,210,16,16,"STATUS:Connected   ");//�����������(UDP�Ƿǿɿ�����,���������ʾ����UDP�Ѿ�׼����)
				POINT_COLOR=RED;
				LCD_ShowString(30,210,lcddev.width-30,lcddev.height-190,16,"Receive Data:");//��ʾ��Ϣ		
				POINT_COLOR=BLUE;//��ɫ����
			}
			else 
				res=1;
		}
		else 
			res=1;		
	}
	else 
		res=1;


	while(res==0)
	{
		if(udp_demo_flag&1<<6)//�Ƿ��յ�����?
		{
			//LCD_Fill(30,230,lcddev.width-1,lcddev.height-1,WHITE);//����һ������
			//LCD_ShowString(30,230,lcddev.width-30,lcddev.height-230,16,udp_demo_recvbuf);//��ʾ���յ�������			
			
			//�Ѿ����յ���һ��������UDP����֡,ץȡ֡ͷ��Ϣ������,������У��,ͬʱ����У��֡
			FrameByteNum = 0;
			Command = udp_demo_recvbuf[3];
			FrameNumber = udp_demo_recvbuf[4];
			DataLength = (((u16)udp_demo_recvbuf[5])<<8) + (u16)udp_demo_recvbuf[6];

			for (j = 0; j <= DataLength + 6; j++)
			{
				CheckSum += udp_demo_recvbuf[j];
			}
			CheckSumL = (u8)CheckSum;
			CheckSumH = (u8)(CheckSum >> 8);
		
			//�ж����У���֡����Ƿ���ȷ
			if((udp_demo_recvbuf[DataLength + 7] == CheckSumH) && (udp_demo_recvbuf[DataLength + 8] == CheckSumL) && \
		  	((FrameNumber == LastFrameNum)||(FrameNumber == LastFrameNum + 1)))  
			{
				//�����ݴ浽FileBuff��
				for(i = FileNum; i < FileNum + DataLength; i++)
				{
					FileBuff[i] = udp_demo_recvbuf[i-FileNum+7];
				}
				FileNum += DataLength;
				LastFrameNum++;

				//Ӧ��֡У��
				ACKCheckSum = FrameFlag + SrcAdd + DestAdd + Command + FrameNumber;
				ACKCheckSumL = (u8)ACKCheckSum;
				ACKCheckSumH = (u8)(ACKCheckSum >> 8);
				//����Ӧ��֡
				udp_demo_sendbuf[0] = FrameFlag;
				udp_demo_sendbuf[1] = SrcAdd;
				udp_demo_sendbuf[2] = DestAdd;
				udp_demo_sendbuf[3] = Command;
				udp_demo_sendbuf[4] = FrameNumber;
				udp_demo_sendbuf[5] = 0;
				udp_demo_sendbuf[6] = 0;
				udp_demo_sendbuf[7] = ACKCheckSumH;
				udp_demo_sendbuf[8] = ACKCheckSumL;

				udp_demo_senddata(udppcb);
			}

			//���У��λ�����ȱ�ʾλ
			CheckSumH = 0;
			CheckSumL = 0;
			CheckSum = 0;
			DataLength = 0;

			udp_demo_flag&=~(1<<6);//��������Ѿ���������
			
		} 
		lwip_periodic_handle();
		delay_ms(2);
		t++;
		if(t==200)
		{
			t=0;
			LED0=!LED0;				//LED0��ʶUDP������������
		}
		if(Command == 0X02)
			break;
	}

	if(Command == 0X02)
	{
		LCD_display();
		Command = 0;
	}

	udp_demo_connection_close(udppcb); 
	myfree(SRAMIN,tbuf);
} 


//UDP�������ص�����
void udp_demo_recv(void *arg,struct udp_pcb *upcb,struct pbuf *p,struct ip_addr *addr,u16_t port)
{
	u32 data_len = 0;
	struct pbuf *q;
	if(p != NULL)	//���յ���Ϊ�յ�����ʱ
	{
		memset(udp_demo_recvbuf,0,UDP_DEMO_RX_BUFSIZE);  //���ݽ��ջ���������
		for(q=p;q!=NULL;q=q->next)  //����������pbuf����
		{
			//�ж�Ҫ������UDP_DEMO_RX_BUFSIZE�е������Ƿ����UDP_DEMO_RX_BUFSIZE��ʣ��ռ䣬�������
			//�Ļ���ֻ����UDP_DEMO_RX_BUFSIZE��ʣ�೤�ȵ����ݣ�����Ļ��Ϳ������е�����
			if(q->len > (UDP_DEMO_RX_BUFSIZE-data_len)) memcpy(udp_demo_recvbuf+data_len,q->payload,(UDP_DEMO_RX_BUFSIZE-data_len));//��������
			else memcpy(udp_demo_recvbuf+data_len,q->payload,q->len);
			data_len += q->len;  	
			if(data_len > UDP_DEMO_RX_BUFSIZE) break; //����UDP�ͻ��˽�������,����	
		}
		upcb->remote_ip=*addr; 				//��¼Զ��������IP��ַ
		upcb->remote_port=port;  			//��¼Զ�������Ķ˿ں�
		lwipdev.remoteip[0]=upcb->remote_ip.addr&0xff; 		//IADDR4
		lwipdev.remoteip[1]=(upcb->remote_ip.addr>>8)&0xff; //IADDR3
		lwipdev.remoteip[2]=(upcb->remote_ip.addr>>16)&0xff;//IADDR2
		lwipdev.remoteip[3]=(upcb->remote_ip.addr>>24)&0xff;//IADDR1 
		udp_demo_flag|=1<<6;				//��ǽ��յ�������
		pbuf_free(p);//�ͷ��ڴ�
	}
	else
	{
		udp_disconnect(upcb); 
	} 
} 
//UDP��������������,�ú������ڷ���Ӧ��֡
void udp_demo_senddata(struct udp_pcb *upcb)
{
	struct pbuf *ptr;
	ptr=pbuf_alloc(PBUF_TRANSPORT, 9, PBUF_POOL); //�����ڴ�
	if(ptr)
	{
		ptr->payload=(void*)udp_demo_sendbuf; 	//���͵�����������
		udp_send(upcb,ptr);	//udp�������� 
		pbuf_free(ptr);//�ͷ��ڴ�
	} 
} 
//�ر�UDP����
void udp_demo_connection_close(struct udp_pcb *upcb)
{
	udp_disconnect(upcb); 
	udp_remove(upcb);		//�Ͽ�UDP���� 
}

//LCD��ʾ
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
























