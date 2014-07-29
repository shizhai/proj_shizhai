/*****************************************************
*copyright (c) 2014.4.9 yangshi (ysprogram@163.com)
*this is a free souce program,you can redistribute it 
*and/or modify it by the Free Software Foundation
*
*****************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kmod.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/random.h>
#include <linux/moduleparam.h>

#include <linux/zw_security.h>

#include "sha256.h"

#define MAJOR_NUM 	0 				//为0时表示动态分配

#define GPIO_PORT_SEND          14

#define DEVICE_NAME	"zw_encry"
#define label_send      "encry_gpio_send"

/*IO Flags*/
#define IO_FLAG_COMMAND		0X77
#define IO_FLAG_TRANSMIT	0X88
#define IO_FLAG_SLEEP		0Xcc

/****************************************************
*消息块challenge操作码,该消息体包括：
*macOp Mode KeyID Challengge
*该发送消息块结构：
*count Opcode Mode KeyID Challenge CRC-16;其中count
*包括自身1Byte与CRC 2个Byte
****************************************************/
#define OPCODE_MAC   (0x08)				//发送到芯片MAC消息体命令

/*芯片属性参数读取命令*/
#define OPCODE_READ	(0x02)				//读取命令
#define OPCODE_BURN_FUSE	(0x04)
#define OPCODE_GEN_PERS		(0x20)
#define OPCODE_BURN_SECURE	(0x10)
#define OPCODE_PAUSE_LONG	(0x01)


/*delay better use kernel timer*/
#define DELAY_Twlo	udelay(70)			//唤醒低电平延时70us,标准>60us
#define DELAY_Twhi	mdelay(5)			//唤醒高电平延时5ms,标准[2.5-45]ms
#define DELAY_TTstart	udelay(4)			//主机发送起始标志低电平延时4.2us
#define DELAY_TFstart	udelay(6)			//主机接收起始标志低电平延时6us
#define DELAY_TTzhi	udelay(4)			//主机发送0的Tzhi高电平延时4.2us
#define DELAY_TFzhi	udelay(6)			//主机接收0的Tzhi高电平延时6us
#define DELAY_TTzlo	udelay(4)			//主机发送0的Tzlo低电平延时4.2us
#define DELAY_TFzlo     udelay(6)               	//主机接收0的Tzlo低电平延时6us
#define DELAY_BITS	udelay(40)			//主机发送1BIT时间39us
#define DELAY_BITC	udelay(50)			//主机接收1BIT时间54us
#define DELAY_TBITOP	udelay(35)			//主机发送1补充延时39-4
#define DELAY_TBITZP	udelay(26)			//主机发送0补充延时39-13.2
#define TIME_OUT	50


#define DELAY_EXEC_READ		1			//接近3ms
#define DELAY_EXEC_MAC		2			//25ms
#define DELAY_PARSE		3			//100us
#define DELAY_EXEC_FUSE		4			//700us
#define DELAY_EXEC_SECURE	5			//35ms
#define DELAY_PERSON		6			//13ms

/*调试使用定义*/
#define deBug 0
#if deBug
#define PRINTK(...)     printk(__VA_ARGS__)
#else
#define PRINTK(...)   mdelay(1);udelay(500)
#endif


static int encry_gpio_MAJOR = MAJOR_NUM;

//加入linux/device.h linux/cdev.h等自动生成/dev/*
static  struct cdev cdev_encry_gpio;			//定义一个cdev结构体变量
struct class *class_encry_gpio;				//定义一个class结构体指针变量


/*256bit密钥,atmel厂商给定，我们可以决定的是64bit的安全熔丝*/
static uchar key[32]={                          	//必先参数，烧写到芯片
        0x26,0xc7,0x66,0x4d,0x22,0xf1,0x5b,0x8b,
        0xcc,0x4d,0x30,0x2b,0x5e,0xc9,0x8b,0x61,
        0xab,0x94,0x86,0xdf,0x66,0xd9,0xc4,0x12,
        0xe7,0x20,0x6e,0xf4,0x02,0x52,0x0e,0x55};
/*64bit安全熔丝*/
static uchar fuse_key[8]={
	0xd4,0xda,0xcd,0xfb,0xbe,0xfc,0xc6,0xb7		//密码数据为：众望军品
        };  

/*256bit随机数*/
uchar mychallenge[32];                                  //必选参数，发送到芯片

/****************************************************
*Fuse[87] unburned & Mode is 0x00 mean that not 
*contain 48-bit serial number(ROM SN-16bit;
*Fuse SN-32bit) and 64bit secret fuses and 24 status 
*fuses, all these replaced with 0's
****************************************************/
static const uchar msgmode = 0x50;			//必选参数，发送到芯片

static const uchar KeyID[2]={0xff,0xff};             	//必选参数(可选择其它，ffff为测试，但是atmel只为每个客户提供一个key)，发送到芯片

/*24bit status fuses*/
static uchar StatusFuse[3]={                            //可选参数，烧写，可读
        0x00,0x00,0x00};
/*8bit Fuse MfrID*/
static uchar FuseMfrID = 0x77;                          //必选参数，固定（同类型），可读

/*32bit Fuse SN number*/
static uchar FuseSN[4]={                                //可选参数，固定，可读
        0x00,0x00,0x00,0x00};

/*16bit ROM MfrID */
static uchar ROMMfrID[2]={                              //必选参数，固定（同类型），可读
        0xcc,0xdd};

/*16bit ROM SN*/
static uchar ROMSN[2]={                                 //可选参数，固定，可读
        0x00,0x00};


/*屏蔽中断*/
/*在gpio_get_value与gpio_set_value的底层实现中已经屏蔽与打开中断，故在读取数据时候不会出现问题*/
/*/trunk/build_dir/target-mips_r2_uClibc-0.9.33.2/linux-ar71xx_generic/linux-3.10.4/arch/mips/ath79/gpio.h*/
/*/trunk/build_dir/target-mips_r2_uClibc-0.9.33.2/linux-ar71xx_generic/linux-3.10.4/arch/mips/include/asm/mach-ath79/ar71xx_regs.h*/

void protect_zw_encry(void)
{
	printk("pzw\n");
}
EXPORT_SYMBOL(protect_zw_encry);

/*CRC计算*/
static uint crc16(uchar *buf,uchar len)
{
	uint crc = 0x0000;
	uint i;
	uchar cnt,bit,cbit;
	for(cnt = 0; cnt < len; cnt ++)	
	{
		for(i=0x01; i < 0x100; i <<= 1)	
		{
			if(buf[cnt] & (uchar) i)bit =0x01;
			else bit = 0x00;
			if(0x8000 & crc)cbit=0x01;
			else cbit = 0x00;
			if (bit ^ cbit)crc = ((crc << 1) & 0xffff) ^ 0x8005;
			else crc = (crc << 1) & 0xffff;
		}
	}
	return crc;
}

/*唤醒*/
static void wakeup(void)
{
        if(gpio_direction_output(GPIO_PORT_SEND,1) == -ENOSYS)	//1表示初始化输出200ns
        {
                printk("direction output error!\n");
		return ;
        }
	gpio_set_value(GPIO_PORT_SEND,0);
	/*低电平70us*/
	DELAY_Twlo;
	gpio_set_value(GPIO_PORT_SEND,1);			//1.4us
	/*保证高电平10ms*/
	DELAY_Twhi;
        if(gpio_direction_input(GPIO_PORT_SEND) == -ENOSYS)	//把输出切换到输入，防止影响芯片返回拉低,200ns
        {
                printk("direction GPIO_PORT_SEND input error!\n");
		return ;
        }
}

/*发送 0*/
static void send_zeroBit(void)
{
	//拉低
	gpio_set_value(GPIO_PORT_SEND,0);
	/*发送起始低电平4.2us*/
	DELAY_TTstart;
	ndelay(100);
	gpio_set_value(GPIO_PORT_SEND,1);
	/*发送0,高电平4.2us*/
	DELAY_TTzhi;
	ndelay(100);
	gpio_set_value(GPIO_PORT_SEND,0);
	/*发送0，低电平4.2us*/
	DELAY_TTzlo;
	ndelay(100);
	gpio_set_value(GPIO_PORT_SEND,1);
	/*补充bit time*/	
	DELAY_TBITZP;
	//在些设置输出到低电平计算补充延时是否正确 
}

/*发送 1*/
static void send_oneBit(void)
{
	//拉低
        gpio_set_value(GPIO_PORT_SEND,0);
        /*发送起始低电平4.2us*/
        DELAY_TTstart;
	ndelay(100);
	gpio_set_value(GPIO_PORT_SEND,1);
	/*直接延时到bit timie*/
	DELAY_TBITOP;
}

/*发送1个Byte*/
static void send_oneByte(uchar s)
{	
	uchar i ;

        if(gpio_direction_output(GPIO_PORT_SEND,1) == -ENOSYS)	//1表示初始化输出
        {
                printk("direction GPIO_PROT_SEND output error!\n");
		return;
        }
	for(i = 0; i < 8; i++)
	{
		if(s&(1<<i))send_oneBit();
		else send_zeroBit();
	}
        if(gpio_direction_input(GPIO_PORT_SEND) == -ENOSYS)
        {
                printk("direction GPIO_PORT_SEND input error!\n");
		return 0;
        }
}

/*接收 1个bit*/
static long timeout;
static uchar zero_flag;
static uchar receive_oneBit(uchar *r)
{
	/*CPU 533MHZ 每条指令0.002us 等待约80us*/
	timeout = 500000;
	zero_flag = 0;
	/*检测下降沿*/
	do
	{
		if(!gpio_get_value(GPIO_PORT_SEND))break;

	}while(--timeout > 0);
	/*超时*/
	if(timeout <= 0)
	{
		PRINTK("receive_oneBit down error1 \n");
		return 0;
	}
	/*最长等待8us，并且只要没有意外肯定会有上升沿*/
	timeout = 400000;
	/*等待上升沿*/
	do{
		if(gpio_get_value(GPIO_PORT_SEND))
		{
			zero_flag=1;
			break;
		}
	}while(--timeout > 0);
	/*超时*/
	if(timeout <= 0)
	{
		PRINTK("receive_oneBit up error2 \n");
		return 0;
	}
	timeout = 120;
	/*等待第二个下降沿*/
	do{
		if(!gpio_get_value(GPIO_PORT_SEND)){zero_flag=0;break;}
	}while(--timeout > 0);
	
	if(0 == zero_flag)	
		/*一定会变成高电平*/
		while(!gpio_get_value(GPIO_PORT_SEND));
	
	if(1 == zero_flag) *r=0x01;
	else *r=0x00;
	return 1;	
}

/*接收1个Byte*/
static uchar receive_oneByte(uchar *R)
{
	uchar bit,byte=0,i;
	for (i=0;i < 8; i++)
	{
		if(!receive_oneBit(&bit))
		{
			PRINTK("receive_oneByte timeoutx error!\n");
			return 0;
		}
		byte |= (bit << i);
	}
	*R = byte;
	return 1;
}


/*send io sleep command flag 0xcc*/
static void send_SLEEP_flag(void)
{
	send_oneByte(IO_FLAG_SLEEP);
	mdelay(100);
}

/*send io command flags 0x77 & cmd block*/
static inline uchar send_cmd(cmd_buff *cmd, u32 delay)
{
	uchar i;
	cmd->count = cmd->data[0];
	cmd->crc = crc16(&(cmd->data[0]),(cmd->data[0] -2));
	/*先发送16bit CRC 低8位*/
	cmd->data[cmd->count - 2] = cmd->crc & 0xff;
	cmd->data[cmd->count - 1] = (cmd->crc >> 8) & 0xff;
	/*首先发送一个IO_FLAG_COMMAND*/
	send_oneByte(IO_FLAG_COMMAND);
	/*循环发送数据*/
	for(i=0;i < cmd->count;i++)
	{
		send_oneByte(cmd->data[i]);
	}

	/*此处应该传递参数进行延时，因为在READ与MAC等等都不一样的延时*/

	switch(delay)
	{
		case 1:mdelay(4);break;
		case 2:mdelay(30);udelay(100);break;//udelay(100);break;
		case 3:udelay(100);break;
		case 4:udelay(700);break;
		case 5:mdelay(36);break;
		case 6:mdelay(13);break;
		default:break;
	}
	return 1;
}
/*send io transmit flags & recieve respose data block*/
static inline uchar send_transmit(cmd_buff *rep)
{
	uchar tmp[40],i,ii;
	send_oneByte(IO_FLAG_TRANSMIT);	
	udelay(50);//延时28-95us为tTURNAROUND
	/*接收数据长度*/
	i = 1;ii=1;
	while(!receive_oneByte(&tmp[0])&&(i++ < 5));
	for(i=1;i<tmp[0];i++)
	{
		if(!receive_oneByte(&tmp[i]))
		{
			PRINTK("data get error \n");return 0;
		}
	}
	memcpy(&(rep->data[0]),tmp,i);
	mdelay(25);
	rep->count = rep->data[0];
	/*先接收到的是CRC的低8位，总体消息体的发送是从最左位开始发送*/
	rep->crc = crc16(&(rep->data[0]),(rep->data[0] - 2));
	/*CRC校验接收的数据*/
	if(((rep->crc & 0xff)!=rep->data[rep->count - 2]) || (((rep->crc >>8)&0xff)!=rep->data[rep->count - 1]))
	{
		PRINTK("transmit receive error crc!\n");
		return 0;
	}
	return 1;
}
/*命令操作与接收返回*/
static uchar sa_process(cmd_buff *cmd,cmd_buff *rep, u32 delay)
{
	cmd_buff buf;
	if(!cmd || !rep)return 0;
	/*wake up the device*/
	wakeup();
	/*send a transmit flag*/
	if(!send_transmit(&buf))
	{
		PRINTK("send_transmit1 sa_process error\n");
		return 0;
	}
	/*send command*/
	if(!send_cmd(cmd, delay))
	{
		PRINTK("send_cmd sa_process error\n");
		return 0;
	}
	
	/*get response*/
	if(!send_transmit(rep))
	{
		PRINTK("send_transmit2 sa_process error\n");
		return 0;
	}
	/*芯片休眠*/
	send_SLEEP_flag();
	return 1;
}

/*读取ROM芯片信息*/
static uchar read_rom(u32 addr,uchar *buf)
{
        cmd_buff cmd,rep;

        cmd.data[0] = 7;
        cmd.data[1] = OPCODE_READ;
        cmd.data[2] = 0x00;
        cmd.data[3] = addr & 0xff;
        cmd.data[4] = (addr >> 8) & 0xff;

        if(!sa_process(&cmd, &rep, DELAY_EXEC_READ))
        {
                PRINTK("sa_process read_rom1 error\n");
                return 0;
        }
        if(rep.count < 3)
        {

                PRINTK("sa_process read_rom2 error\n");
                return 0;
        }
        /*除掉数据长度1B，CRC 2B*/
        memcpy(buf,&rep.data[1],(rep.count-3));
        return (rep.count-3);
}
/*读取FUSE*/
static uchar read_fuse(u32 addr, uchar *buf)
{
        cmd_buff cmd,rep;

        cmd.data[0] = 7;
        cmd.data[1] = OPCODE_READ;
        cmd.data[2] = 0x01;
        cmd.data[3] = addr & 0xff;
        cmd.data[4] = (addr >> 8) & 0xff;

        if(!sa_process(&cmd, &rep, DELAY_EXEC_READ))
        {
                PRINTK("sa_process read_fuse1 error\n");
                return 0;
        }
        if(rep.count < 3)
        {

                PRINTK("sa_process read_fuse2 error\n");
                return 0;
        }
        /*除掉数据长度1B，CRC 2B*/
        memcpy(buf,&rep.data[1],(rep.count-3));
        return (rep.count-3);
}

/*读取MAC消息返回*/
static uchar read_mac(uchar mode, uint keyid,const uchar *challenge,uchar *mac)
{
	cmd_buff cmd,rep;
	
	cmd.data[0] = 0x27;					//总共39个字节，也是最大字节数
	cmd.data[1] = OPCODE_MAC;
	cmd.data[2] = mode;
	cmd.data[3] = keyid & 0xff;
	cmd.data[4] = (keyid >> 8) & 0xff;
	
	memcpy(&cmd.data[5],challenge,32);

	if(!sa_process(&cmd, &rep, DELAY_EXEC_MAC))
	{
		PRINTK("sa_process read_mac1 error\n");
		return 0;
	}
	if(rep.count < 3)
	{
		
		PRINTK("sa_process read_mac2 error\n");
		return 0;	
	}
	/*除掉数据长度1B，CRC 2B*/
	memcpy(mac,&rep.data[1],(rep.count-3));
	return (rep.count-3);
}

/*do mac ,mac 消息处理及发送*/
uchar do_mac(uchar mode, uint keyid,const uchar * challenge, const uchar * expected)
{
	uchar mac[32];
	uchar len;
	uchar i;

	if(!challenge || !expected)
	{
		PRINTK("do_mac error 1 \n");	
		return 0;
	}

	len = read_mac(mode, keyid, challenge, mac);
	if(len != 32)
	{
		PRINTK("do_mac error 2 \n");	
		return 0;
	}

#if deBug
	/*
	 * Print mac result from chip
	 */
	printk("mac: ", mac[i]);
	for(i = 0; i < len; i++)
	{
		if((i % 8) == 0)
			printk("\r\n");
		printk("0x%02x, ", mac[i]);
	}
	printk("\r\n");
#endif

	for(i = 0; i < len; i++)
	{
		if(mac[i] != expected[i])
		{
			PRINTK("crc don't match!\n");
			return 0;
		}
	}

	return 1;

}

/*******************************************************
*serial number is 48bits / 6Bytes(16 bits from ROM 
*address at 0x00,32bits from fuses address at 0x03)
********************************************************/
static uchar read_serial_number(uchar *buf)
{
        uchar rom[32];
        uchar fuse[32];
        uchar len;

        len = read_rom(0, rom);
        if(len != 4)
        {
                PRINTK("read_serial_number 1 error\n");
                return 0;
        }
        len = read_fuse(3, fuse);
        if(len != 4)
        {
                PRINTK("read_serial_number 2 error\n");
                return 0;
        }

        /*rom 地址0的32bit的后两个字节为序列号*/
        buf[0] = rom[2];
        buf[1] = rom[3];
        buf[2] = fuse[0];
        buf[3] = fuse[1];
        buf[4] = fuse[2];
        buf[5] = fuse[3];

        return 1;
}

/******************************************************
*读取MfrID,包括24bits/3Bytes(16bits from ROM address at
*0x00,8bits from fuses address at 0x0x02)
******************************************************/
static uchar read_manufacture_id(uchar *buf)
{
        uchar rom[32];
        uchar fuse[32];
        uchar len;

        len = read_rom(0, rom);
        if(len != 4)
        {
                PRINTK("read_manufacture_id 1 error\n");
                return 0;
        }

        len = read_fuse(2, fuse);
        if(len != 4)
        {
                PRINTK("read_manufacture_id 2 error\n");
                return 0;
        }

        buf[0] = rom[0];
        buf[1] = rom[1];
        buf[2] = fuse[3];
        return 1;
}
/*read status fuses 3 Bytes*/
static uchar read_fuse_status(uchar *buf)
{
        uchar fuse[32];
        uchar len;

        len = read_fuse(2, fuse);
        if(len != 4)
        {
                PRINTK("read_fuse_status error\n");
                return 0;
        }

        buf[0] = fuse[0];
        buf[1] = fuse[1];
        buf[2] = fuse[2];

        return 1;
}

/*认证主程序*/
uchar msg[88];
uchar test_do_auth(void)
{
	uchar index,i=0;
	uchar buf[64],count=0;

        sha256_context ctx;
        unsigned char sha256sum[32];
	
	/*读取芯片信息并显示*/
	while(1)
	{
		//读取序列号
		count = 0;
		if(read_serial_number(buf))
		{
		#if 0
			printk("\n the serial_number is:\n");
			for(i=0;i<6;i++)printk("0x%02x ",buf[i]);
			printk("\n");
		#else
			mdelay(10);
		#endif
			memcpy(ROMSN,buf,2);
			memcpy(FuseSN,buf+2,4);
			break;
		}
		if(count++ > 5)
		{
			printk(KERN_INFO"error serial no. \n");
			return 0;
		}
		mdelay(500);
	}
	while(1)
	{	
		//读取MfrID
		count = 0;
		if(read_manufacture_id(buf))
		{
		#if 0
			printk("\n the manufacture id is:\n");
			for(i=0;i<3;i++)printk("0x%02x ",buf[i]);
			printk("\n");
		#else
			mdelay(10);
		#endif
			ROMMfrID[0] = buf[0];
			ROMMfrID[1] = buf[1];
			FuseMfrID = buf[2];
			break;
		}
		if(count++ > 5)
		{
			printk(KERN_INFO"error manufacture id \n");
			return 0;
		}
		mdelay(500);
	}

	while(1)
	{	
		//读取状态熔丝
		count = 0;
		if(read_fuse_status(buf))
		{
		#if 0
			printk("\n the status is:\n");
			for(i=0;i<3;i++)printk("0x%02x ",buf[i]);
			printk("\n");
		#else
			mdelay(10);
		#endif		
			StatusFuse[0] = buf[0];
			StatusFuse[1] = buf[1];
			StatusFuse[2] = buf[2];
			break;
		}
		if(count++ > 5)
		{
			printk(KERN_INFO"error status \n");
			return 0;
		}
		mdelay(500);
	}
	/*内核产生随机数*/
	for(count=0;count<32;count++)
	{
		get_random_bytes(&index,sizeof(uchar));
		mychallenge[count] = index%255;
	}
	switch(msgmode)
	{
		case 0x00:memset(fuse_key,0x00,8),memset(StatusFuse,0x00,3);memset(FuseSN,0x00,4);memset(ROMSN,0x00,2);break;
		case 0x10:memset(FuseSN,0x00,4);memset(ROMSN,0x00,2);break;
		case 0x20:memset(StatusFuse,0x00,3);break;
		default:break;
	}
	/*数据聚合*/
	memcpy(msg,key,32);			//内置key，有一个调试使用的key，最后可以向atmel申请一个key(一个用户仅一个)
	memcpy(msg+32,mychallenge,32);		//随机数据
	msg[64] = 0x08;				//操作码
	msg[65] = msgmode;			//模式码，选择serial与fuse key与statusFuse
	memcpy(msg+66,KeyID,2);			//key索引
	memcpy(msg+68,fuse_key,8);		//fuse key,64bit，用户烧写后不能读取，可选
	memcpy(msg+76,StatusFuse,3);		//StatusFfuse，24bit(其实只有23bit，最后一bit为BurnSecure Disable)，用户烧写，可以读取
	msg[79] = FuseMfrID;			//atmel设置，可读，必选
	memcpy(msg+80,FuseSN,4);		//fuse serial，atmel设置，可读
	memcpy(msg+84,ROMMfrID,2);		//atmel设置，可读，必选
	memcpy(msg+86,ROMSN,2);			//atmel设置，可读
#if 0
	for(i=0;i<88;i++)
	{
		printk("msg[%2d] = %02x ",i,msg[i]);
		if((i+1)%8==0)printk("\n");
	}
#else
	mdelay(10);
#endif
	sha256_starts( &ctx );
	sha256_update( &ctx,msg,88);
	sha256_finish( &ctx, sha256sum);
#if 0
	for(i=0;i<32;i++)
	{
		printk("0x%02x, ",sha256sum[i]);
		if((i+1)%8==0)printk("\n");
	}
	printk("\n");	
#else
	mdelay(10);
#endif
	count = 0;	
	while(1)
	{

		get_random_bytes(&index,sizeof(int));
		index = index % 4;
		PRINTK("index is %d \n",index);
		if(do_mac(msgmode, 0xffff, mychallenge,sha256sum))
		{
			printk(KERN_INFO"Authenticated!\n");
			return 1;
		}

		else if(count++>5)
		{
			printk(KERN_INFO"failed\n");
			return 0;
		}
		else 
		{
			mdelay(900);
			PRINTK(KERN_INFO"try again !\n ");
			continue;
		}

	}
}
/******ioctl函数*********/
static long encry_gpio_ioctl(struct file *filp,unsigned int cmd,unsigned long arg)
{
	return 0;
}

static int  encry_gpio_init(void)
{
        int result;
        result=(gpio_request(GPIO_PORT_SEND,label_send));
        if(result)
        {
                printk("err request!\n");
                return 0;
        }
        if(gpio_direction_output(GPIO_PORT_SEND,1) == -ENOSYS)	//1表示初始化输出
        {
                printk("direction output error!\n");
		return 0;
        }
        return 1;
}


/*static int ledgpio_open(struct inode *inode,struct file *filp)
{
	
}*/

static struct file_operations encry_gpio_fops=
{
	.owner = THIS_MODULE,
	.unlocked_ioctl = encry_gpio_ioctl,
};

static int __init ledgpio_init(void)
{
	int result;
	dev_t devno;
	if(encry_gpio_MAJOR) 
	{	
		devno = MKDEV(encry_gpio_MAJOR,0);	//次设备号为0
		result=register_chrdev_region(devno,1,DEVICE_NAME);
	}
	else 
	{
		result = alloc_chrdev_region(&devno,0,1,DEVICE_NAME);
		encry_gpio_MAJOR = MAJOR(devno);
	}
	
	if(result)
	{
		printk(KERN_NOTICE"Error %d register ledgpio",result);
		return result;
	}
	cdev_init(&cdev_encry_gpio,&encry_gpio_fops);			//对空间起到一个清零的作用
	result = cdev_add(&cdev_encry_gpio,devno,1);
	if(result)
	{
		printk(KERN_NOTICE"Error %d a ding signal_gpio",result);
		return result;
	}
	class_encry_gpio = class_create(THIS_MODULE,"class_encry_gpio");
	/*创建一个设备并且将其在sysfs中注册*/
	device_create(class_encry_gpio,NULL,MKDEV(encry_gpio_MAJOR,0),"NULL",DEVICE_NAME);
	
	//驱动设备系统初始化完毕
	//初始化GPIO14与GPIO15
	if(!encry_gpio_init())
	{
		printk(KERN_NOTICE"single_gpio_init error!\n");
		return 0;
	}
	//进行加密控制
	/*产生一个随机数据256bit与key 256bit异或进行摘要计算，把随机数据传输到加密芯片*/
	if(test_do_auth())return 0;
	else while(1);							//如果认证失败，死循环等待重启 
}

/*设备驱动模块卸载函数*/
static void __exit ledgpio_exit(void)
{
	cdev_del(&cdev_encry_gpio);
	/*当一个字符设备驱动不再需要的时候，比如模块卸载就是可以用cdev_del()函数来释放cdev占用的内存*/
	unregister_chrdev_region(MKDEV(encry_gpio_MAJOR,0),1);//注销设备区域
	device_destroy(class_encry_gpio,MKDEV(encry_gpio_MAJOR,0));
	/*删除处于/dev目录下手驱动节点*/
	class_destroy(class_encry_gpio);
	/*删除我们所创建的class*/
        /*释放占用GPIO*/
	gpio_free(GPIO_PORT_SEND);
}

core_initcall(ledgpio_init);
//subsys_initcall(ledgpio_init);
//module_init(ledgpio_init);
//module_exit(ledgpio_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("SHIZHAI");
