#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include "def.h"
#include "common.h"

static struct itimerval oldtv;

#define deBug 1
#if deBug
#define PRINTF(...)	{printf(__VA_ARGS__);printf("\n");}
#else
#define PRINTF(...)	{;}
#endif

/*****************************************************************
*定时更新时间操作
*****************************************************************/
void set_timer()
{
	struct itimerval itv;
	itv.it_interval.tv_sec = 59; 
	itv.it_interval.tv_usec = 500;
	itv.it_value.tv_sec = 1;
	itv.it_value.tv_usec = 0;
	setitimer(0, &itv, &oldtv);
}

/***************************************************
*线程每一分钟更新一次时间
***************************************************/
void timeUp()
{
	uchar t[30];
	uchar timeNow[2];
	uchar ll;
	time_t now;
	now = time(NULL);
	sprintf(t,"%s",ctime(&now));
	ll = strlen(t);
	timeGet[0] = ((t[strlen(t)-14]-0x30)<<4)|(t[strlen(t)-13]-0x30);
	timeGet[1] = ((t[strlen(t)-11]-0x30)<<4)|(t[strlen(t)-10]-0x30);
	timeIsUpdate = 1;
	timeIsUpdate1 = 1;
	timeIsUpdate2 = 1;
	PRINTF("the time is updated!\n");
}

/*****************************************************************
*参数格式：
*灯号 操作1 时间1（时，分）参数 操作2 时间2（时，分）参数 操作3 时间3 FC 灯号
*到FC就表示第一盏灯设置结束
*在定时的时候参数为零，在morning call 与 呼吸灯的时候为间隔
******************************************************************/

void timeproc(uchar *timeSet,uint len)
{
	struct lightx
	{
		uchar timeS;					//表示该灯有几个时间段
		uchar timeSection[lightProc][2];		//有lightProc个时间段，每个为：时分
		uchar  lightProcCmd[lightProc][7];
	}light[lightNum];					//预设100盏灯
	uchar get_ackt[NUMR];
	uchar num = 0,i = 0,j = 0,k = 0;
	uchar lightN = 0;
	
	uchar temp[64];
	lightN = 0;

	bzero(light,lightNum);					//清空空间
	for(num=0;num < len;num++)
	{	
		if(timeSet[num] != 0xFC)
		{
			temp[i++] = timeSet[num];
		}
		else						//统计灯数
		{
			i /= 4;
			light[lightN].timeS = i;
			for(j=0;j<light[lightN].timeS;j++)
			{
				light[lightN].timeSection[j][0] = temp[j*4+2];
				light[lightN].timeSection[j][1] = temp[j*4+3];
				light[lightN].lightProcCmd[j][0] = 0xFE;
				light[lightN].lightProcCmd[j][1] = 0x02;
				light[lightN].lightProcCmd[j][2] = 0x00;
				light[lightN].lightProcCmd[j][3] = 0x00;
				light[lightN].lightProcCmd[j][4] = temp[j*4+1];						//操作
				light[lightN].lightProcCmd[j][5] = temp[0];						//灯号
				light[lightN].lightProcCmd[j][6] = 0x00;
				for(k=1;k<6;k++)
					light[lightN].lightProcCmd[j][6] ^= light[lightN].lightProcCmd[j][k];		//生成校验
			}
			i = 0;
			lightN ++;
		}
	}
	while(timeOn)
	{
		while(!timeIsUpdate)sleep(1);				//如果时间没有更新就睡一秒再循环再检测
		timeIsUpdate = 0;					//时间恢复到没有更新
		for(i=0;i<=lightN;i++)
		{	
			for(j=0;j<light[i].timeS;j++)
			{
				if((timeGet[0]==light[i].timeSection[j][0])&&(timeGet[1]==light[i].timeSection[j][1]))
				{
					for(k=0;k<light[i].lightProcCmd[j][1]+5;k++)
					{	
						temp[k] = light[i].lightProcCmd[j][k];
					}
					commanT(temp,get_ackt);
					PRINTF("haha !\n");
				}
			}
		}
	}
}
/*****************************************************************
*参数格式：
*灯号 操作1 时间1（时，分）参数 操作2 时间2（时，分）参数 操作3 时间3 FC 灯号
*到FC就表示第一盏灯设置结束
*在定时的时候参数为零，在morning call 与 呼吸灯的时候为间隔
******************************************************************/

void timeproc1(uchar *timeSet,uint len)
{
	struct lightx
	{
		uchar timeS;					//表示该灯有几个时间段
		uchar timeSection[lightProc][2];		//有lightProc个时间段，每个为：时分
		uchar  lightProcCmd[lightProc][10];
	}light[lightNum];					//预设100盏灯
	uchar get_ackm[NUMR];
	uchar num = 0,i = 0,j = 0,k = 0;
	uchar lightN = 0;
	uchar temp[64];
	lightN = 0;

	for(num=0;num < len;num++)
	{	
		if(timeSet[num] != 0xFC)
		{
			temp[i++] = timeSet[num];
		}
		else						//统计灯数
		{
			i /= 4;
			light[lightN].timeS = i;
			for(j=0;j<light[lightN].timeS;j++)
			{
				light[lightN].lightProcCmd[j][0] = 0xFE;
				light[lightN].lightProcCmd[j][1] = 0x05;
				light[lightN].lightProcCmd[j][2] = 0x00;
				light[lightN].lightProcCmd[j][3] = 0x00;
				if(temp[j*4+4]>=0x01&&(temp[j*4+4]<=0xf0))							//步隙1~240
				{
					light[lightN].timeSection[j][0] = temp[j*6+2];
					light[lightN].timeSection[j][1] = temp[j*6+3];
					light[lightN].lightProcCmd[j][4] = temp[j*6+1];						//操作
					light[lightN].lightProcCmd[j][5] = temp[0];						//灯号
					light[lightN].lightProcCmd[j][6] = temp[j*6+4];						//步长
					light[lightN].lightProcCmd[j][7] = temp[j*6+5];						//上限
					light[lightN].lightProcCmd[j][8] = temp[j*6+6];						//下限 
					light[lightN].lightProcCmd[j][9] = 0x00;
					for(k=1;k<9;k++)
						light[lightN].lightProcCmd[j][9] ^= light[lightN].lightProcCmd[j][k];		//生成校验
				}
			}
			i = 0;
			lightN ++;
		}
	}

	while(morningOn)
	{
		while(!timeIsUpdate1)sleep(1);				//如果时间没有更新就睡一秒再循环再检测
		timeIsUpdate1 = 0;					//时间恢复到没有更新
		for(i=0;i<=lightN;i++)
		{
			for(j=0;j<light[i].timeS;j++)
			{
				if((timeGet[0]==light[i].timeSection[j][0])&&(timeGet[1]==light[i].timeSection[j][1]))
				{
					for(k=0;k<light[i].lightProcCmd[j][1]+5;k++)
					{
						temp[k] = light[i].lightProcCmd[j][k];
					}
					PRINTF("1 the temp is %x,%x,%x,%x,%x,%x,%x,%x\n",temp[0],temp[1],temp[2],temp[3],temp[4],temp[5],temp[6],temp[7]);
					commanT(temp,get_ackm);
					PRINTF("haha\n");
				}
			}
		}
	}
}

/*****************************************************************
*参数格式：
*灯号 操作1 时间1（时，分）参数 操作2 时间2（时，分）参数 操作3 时间3 FC 灯号
*到FC就表示第一盏灯设置结束
*在定时的时候参数为零，在morning call 与 呼吸灯的时候为间隔
******************************************************************/
void timeproc2(uchar *timeSet,uint len)
{
	struct lightx
	{
		uchar timeS;					//表示该灯有几个时间段
		uchar timeSection[lightProc][2];		//有lightProc个时间段，每个为：时分
		uchar  lightProcCmd[lightProc][8];
	}light[lightNum];					//预设100盏灯
	uchar get_ackb[NUMR];
	uchar num = 0,i = 0,j = 0,k = 0;
	uchar lightN = 0;
	
	uchar temp[64];
	lightN = 0;
	for(num=0;num < len;num++)
	{	
		if(timeSet[num] != 0xFC)
		{
			temp[i++] = timeSet[num];
		}
		else						//统计灯数
		{
			i /= 4;
			light[lightN].timeS = i;
			PRINTF("the light[1].timeS is %d\n",light[lightN].timeS);
			for(j=0;j<light[lightN].timeS;j++)
			{
				light[lightN].lightProcCmd[j][0] = 0xFE;
				light[lightN].lightProcCmd[j][2] = 0x00;
				light[lightN].lightProcCmd[j][3] = 0x00;
				if(temp[j*4+4]>=0x01&&(temp[j*4+4]<=0xf0))							//步隙1~240
				{
					light[lightN].timeSection[j][0] = temp[j*4+2];
					light[lightN].timeSection[j][1] = temp[j*4+3];
					light[lightN].lightProcCmd[j][1] = 0x03;
					light[lightN].lightProcCmd[j][4] = temp[j*4+1];						//操作
					light[lightN].lightProcCmd[j][5] = temp[0];						//灯号
					light[lightN].lightProcCmd[j][6] = temp[j*4+4];
					light[lightN].lightProcCmd[j][7] = 0x00;
					for(k=1;k<7;k++)
						light[lightN].lightProcCmd[j][7] ^= light[lightN].lightProcCmd[j][k];		//生成校验
				}
			}
			i = 0;
			lightN ++;
		}
	}
	while(breatheOn)
	{
		while(!timeIsUpdate2)sleep(1);				//如果时间没有更新就睡一秒再循环再检测
		timeIsUpdate2 = 0;					//时间恢复到没有更新
		for(i=0;i<=lightN;i++)
		{	
			for(j=0;j<light[i].timeS;j++)
			{
				if((timeGet[0]==light[i].timeSection[j][0])&&(timeGet[1]==light[i].timeSection[j][1]))
				{
					for(k=0;k<light[i].lightProcCmd[j][1]+5;k++)
					{
						temp[k] = light[i].lightProcCmd[j][k];
					}
					PRINTF("2 the temp is %x,%x,%x,%x,%x,%x,%x\n",temp[0],temp[1],temp[2],temp[3],temp[4],temp[5],temp[6]);
					commanT(temp,get_ackb);
					PRINTF("haha !\n");
				}
			}
		}
	}
}

