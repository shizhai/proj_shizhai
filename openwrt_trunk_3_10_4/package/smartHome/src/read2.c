#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "common.h"
#include "read.h"

#define deBug 1
#if deBug
#define PRINTF(...)	{printf(__VA_ARGS__);printf("\n");}
#else
#define PRINTF(...)	{;}
#endif

unsigned char get_1[7]={0x00,0x00,0x00,0x00,0x00,0x00,0x00};
int fp;
/***************************************************
*串口打开以及初始化，参数可设置
***************************************************/
void get_myall(void) { 
	
	unsigned char i = 0,check = 0x00;
	FILE *fp1;
	
	struct termios options;

	fp = open("/dev/ttyATH0",O_RDONLY); 				
	ioctl(fp,TIOCCONS);						//TIOCCONS的作用是使成为虚拟控制台
	close(fp); 

	fp = open("/dev/ttyATH0",O_RDWR|O_NOCTTY|O_NDELAY); 	
	if(fp == -1) exit(0);
	if ( tcgetattr ( fp , &options ) !=0 )exit(0);

	tcflush(fp,TCIOFLUSH);

	options.c_cflag |= (CLOCAL|CREAD);

	options.c_cflag &= ~CSIZE ;
	options.c_cflag |= CS8 ;

	cfsetispeed(&options,B115200);
	cfsetospeed(&options,B115200);

	options.c_cflag &= ~PARENB ;
	options.c_iflag &= ~INPCK  ;

	options.c_cflag &= ~CSTOPB ;

	options.c_cflag &= ~CRTSCTS;
	options.c_iflag &= ~(IXON | IXOFF | IXANY);

	options.c_cc[VTIME] = 0 ;
	options.c_cc[VMIN] = 7 ;				//最少接收7个数据 

	/*raw model*/
	options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag  &= ~OPOST;

	options.c_iflag &= ~(INLCR|IGNCR|ICRNL);
	options.c_iflag &= ~(ONLCR|OCRNL);

	options.c_oflag &= ~(INLCR|IGNCR|ICRNL);
	options.c_oflag &= ~(ONLCR|OCRNL);

	tcflush(fp,TCIOFLUSH) ;
	if (tcsetattr(fp,TCSANOW,&options) != 0 )
	{
		perror("Set the terminal error");
		tcsetattr(fp,TCSANOW,&options);
		return  ;
	}

	//close(fp); 

	//fp = open("/dev/ttyATH0",O_RDONLY);  					
	//ioctl(fp,TIOCCONS);							//恢复到ttyATH0作为控制台
	//close(fp);                      						
}
/***************************************************
*通过串口传送命令
***************************************************/
void commanT(uchar *Re,uchar *ack)
{
	int i,j,k,m,n=0;
	get_myall();
	while(1)
	{
		k=0,m=0;
		write(fp,Re,Re[1]+5);						//发送命令
		PRINTF("i am good at commanT Recv is %x,%x,%x,%x,%x,%x,%x,%x",Re[0],Re[1],Re[2],Re[3],Re[4],Re[5],Re[6],Re[7]);
		while(read(fp,ack,NUMR)<ack[1]+5)				//如果收到小于收到字节的长度就不对
		{
			for(i=0;i<2550;i++)for(j=0;j<255;j++);
			if(k++==2){m=1;return;}
			PRINTF("getting ack,please wait...");
		}
		if(m&&n++<2){PRINTF("failed to get ack,try again...");continue;}
		else if(3==n){bzero(ack,NUMR);PRINTF("failed to get ack,the light may be off!");return;}
		else {PRINTF("success to get ack ");break;}
	}
}
