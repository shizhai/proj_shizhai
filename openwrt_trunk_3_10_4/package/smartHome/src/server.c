#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <pthread.h>
#include <time.h>

#include "read.h"
#include "common.h"
#include "def.h"
#include "wifi.h"

#define deBug 1
#if deBug
#define PRINTF(...)	{printf(__VA_ARGS__);printf("\n");}
#else
#define PRINTF(...)	{;}
#endif

struct sockaddr_in my_addr; 			/* 本机地址信息*/
struct sockaddr_in remote_addr; 		/* 客户端地址信息 */ 


/**************************************************
* /www/cgi-bin/isTime:存放定时是否开启
* /www/cgi-bin/isMorning:存放morning功能是否开启
* /www/list :存放当前在线灯号
* /www/cgi-bin/time.cfg:存放定时设置
* /www/cgi-bin/morning.cfg:存放morning call相关配置
* /www/user.cfg:存放用户名，及密码
* /www/state.cfg:存放上次关机的灯具的状态 
***************************************************/

void dataInit()
{
	userNumNow = 0;				//当前用户数量初始化为0
	isCover = 0;				//是否覆盖，初始化为否
	timeIsUpdate = 0;			//初始状态为时间没有更新
	timeIsUpdate1 = 0;
	timeIsUpdate2 = 0;
}

/***************************************************
*用户文档读取检查
***************************************************/
void userCheck()
{
	FILE *userf;
	uchar i = 0,j=0;
	uint len;
	uchar temp[userOfLen+passOfLen];
	userNumNow = 0;
	userf=fopen("/www/cgi-bin/user.cfg","r");
	if(NULL==userf)
	{
		perror("lost user.cfg");return;
	}
	else
	{	
		fseek(userf,0L,SEEK_END);
		len = ftell(userf);
		rewind(userf);
		while((len--)>0)					//文件没有到达结尾部分
		{
			temp[i] = fgetc(userf);
			if(temp[i++]=='\n')
			{
				for(j=0;j<i-1;j++)
				{
					if(temp[j]==' ')break;
					userStore[userNumNow].users[j] = temp[j];
				}
				temp[i-1]='\0';
				strcpy(temp,&temp[j+1]);
				strcpy(userStore[userNumNow].passwords,temp);
				userNumNow++;				//用户增加一个
				i=0;
			}
		}
		fclose(userf);
	}
}


/***************************************************
*定时,呼吸灯,morning call
***************************************************/
void* timming()
{
	int flen1 = 0;
	int ret;
	FILE *timeCfg;
	uchar timeSetCfg[MAXL];
	timeCfg = fopen("/www/cgi-bin/time.cfg","r");
	if(timeCfg != NULL)
	{
		fseek(timeCfg,0L,SEEK_END);
		flen1 = ftell(timeCfg);
		rewind(timeCfg);
		fread(timeSetCfg,SIZEOF(UC),flen1,timeCfg);		//不能读取的长度超过文件实际长度，否则读完后文件被删除
		fclose(timeCfg);
		//PRINTF("the timeSetCfg is:%x,%x,%x,%x,%x,and the flen1 is :%d\n",timeSetCfg[0],timeSetCfg[1],timeSetCfg[2],timeSetCfg[3],timeSetCfg[4],flen1);
		if(flen1<6)
		{
			perror("no time.cfg detail\n");
			timeOn = 0;
			return;
		}
		timeproc(timeSetCfg,flen1);
	}
}
/***************************************************
*定时,呼吸灯,morning call
***************************************************/
void* morning()
{
	int flen1 = 0;
	int ret;
	FILE *morningCfg;
	uchar morningSetCfg[MAXL];
	morningCfg = fopen("/www/cgi-bin/morning.cfg","r");
	if(morningCfg != NULL)
	{
		fseek(morningCfg,0L,SEEK_END);
		flen1 = ftell(morningCfg);
		rewind(morningCfg);
		fread(morningSetCfg,SIZEOF(UC),flen1,morningCfg);		//不能读取的长度超过文件实际长度，否则读完后文件被删除
		fclose(morningCfg);
		if(flen1<6)
		{
			perror("no morning.cfg detail\n");
			morningOn = 0;
			return;
		}
		timeproc1(morningSetCfg,flen1);
	}	
	else
	{
		perror("in the morning() can't open morning.cfg");
	}
}
/***************************************************
*定时,呼吸灯,morning call
***************************************************/
void* breathing()
{
	int flen1 = 0;
	int ret;
	FILE *breatheCfg;
	uchar breatheSetCfg[MAXL];
	breatheCfg = fopen("/www/cgi-bin/breathe.cfg","r");
	if(breatheCfg != NULL)
	{
		fseek(breatheCfg,0L,SEEK_END);
		flen1 = ftell(breatheCfg);
		rewind(breatheCfg);
		fread(breatheSetCfg,SIZEOF(UC),flen1,breatheCfg);		//不能读取的长度超过文件实际长度，否则读完后文件被删除
		fclose(breatheCfg);
		if(flen1<6)
		{
			perror("no breathe.cfg detail\n");
			breatheOn = 0;
			return;
		}
		timeproc2(breatheSetCfg,flen1);
	}
	else
	{
		perror("in the breathing can't open breathe.cfg");
	}
}

/***************************************************
*判断定时等相关功能是否开启
***************************************************/
void TJudge(uchar s)
{
	FILE *isTime,*isMorning,*isBreathe;
	uchar isTimeGet,isMorningGet,isBreatheGet;
	pthread_t timeID1,morningID1,breatheID1;
	int ret = 0;
	/*定时开启与否开机判断*/
	/*注意，可以使用带标志符的open函数，这样就可以使用O_CREAT|O_EXCL来判定文件是否存在*/
	isTime = fopen("/www/cgi-bin/isTime","r");			//isTime存放定时是否开启

	if(isTime!=NULL)
	{
		fread(&isTimeGet,SIZEOF(UC),1,isTime);			//不能读取的长度超过文件实际长度，否则读完后文件被删除
		fclose(isTime);
		if(isTimeGet == 0x01)					//fwrite写入的是什么，读出来的就是什么			
		{
PRINTF("the timming is on\n");
			timeOn = 1;

			ret = pthread_create(&timeID1,NULL,(void*)timming,NULL);
			if(ret !=0 )
			{
				perror("timeID1 pthread_creat error\n");
				return ;
			}
		}
		else
		{
PRINTF("the timming is off\n");
			timeOn = 0;
		}
	}
	else
	{
		perror("can't open isTime\n");
		return;
	}
	/*morning call 开机判断*/
	isMorning=fopen("/www/cgi-bin/isMorning","r");
	if(isMorning!=NULL)
	{
		fread(&isMorningGet,SIZEOF(UC),1,isMorning);			//不能读取的长度超过文件实际长度，否则读完后文件被删除
		fclose(isMorning);
		if(isMorningGet == 0x01)					//fwrite写入的是什么，读出来的就是什么			
		{
PRINTF("the morning is on\n");
			morningOn = 1;

			ret = pthread_create(&morningID1,NULL,(void*)morning,NULL);
			if(ret !=0 )
			{
				perror("morningID1 pthread_creat error\n");
				return ;
			}
		}
		else
		{
PRINTF("the morning is off\n");
			morningOn = 0;
		}
	}
	else
	{
		perror("can't open isMorning morning call 开机判断");
		return;
	}
	/*breathe 开机判断*/
	isBreathe=fopen("/www/cgi-bin/isBreathe","r");
	if(isMorning!=NULL)
	{
		fread(&isBreatheGet,SIZEOF(UC),1,isBreathe);			//不能读取的长度超过文件实际长度，否则读完后文件被删除
		fclose(isBreathe);
		if(isBreatheGet == 0x01)					//fwrite写入的是什么，读出来的就是什么			
		{
PRINTF("the morning is on\n");
			breatheOn = 1;

			ret = pthread_create(&breatheID1,NULL,(void*)breathing,NULL);
			if(ret !=0 )
			{
				perror("breatheID1 pthread_creat error\n");
				return ;
			}
			
		}
		else
		{
PRINTF("the breathe is off\n");
			breatheOn = 0;
		}
	}
	else
	{
		perror("can't open isBreathe\n");
		return;
	}
}


/***************************************************
*测试创建文件isTime.....time.cfg
***************************************************/ 
void testCreat()
{
	FILE *test,*test1;
	uchar timeOff = 0x00;
	uchar rootP[30]="root 123456\nyangshi 123456\n";

	if(NULL==(test1=fopen("/www/cgi-bin/isTime","r")))		//判断定时功能是否开启文档是否存在
	{
		test = fopen("/www/cgi-bin/isTime","w");
		if(test==NULL)
		{
			perror("isTime \n");
			exit(1);
		}
		fwrite(&timeOff,SIZEOF(UC),1,test);
		fclose(test);
	}
	else
	{
		fclose(test1);
	}

	if(NULL==(test1=fopen("/www/cgi-bin/time.cfg","r")))		//判断定时配置文档是否存在
	{
		test = fopen("/www/cgi-bin/time.cfg","w");
		if(test==NULL)
		{
			perror("time.cfg test_create");
			exit(1);
		}
		fwrite(&timeOff,SIZEOF(UC),1,test);
		fclose(test);
	}
	else
	{
		fclose(test1);
	}

	if(NULL==(test1=fopen("/www/list","r")))			//判断灯具列表是否存在
	{
		test = fopen("/www/list","w");
		if(test==NULL)
		{
			perror("list \n");
			exit(1);
		}
		fwrite(&timeOff,SIZEOF(UC),1,test);
		fclose(test);
	}
	else
	{
		fclose(test1);
	}

	if(NULL==(test1=fopen("/www/cgi-bin/isMorning","r")))
	{
		test = fopen("/www/cgi-bin/isMorning","w");			
		if(test==NULL)
		{
			perror("isMorning -test_create");
			exit(1);
		}
		fwrite(&timeOff,SIZEOF(UC),1,test);
		fclose(test);
	}
	else
	{
		fclose(test1);
	}

	if(NULL==(test1=fopen("/www/cgi-bin/isBreathe","r")))
	{
		test = fopen("/www/cgi-bin/isBreathe","w");
		if(test==NULL)
		{
			perror("isBreathe test_create\n");
			exit(1);
		}
		fwrite(&timeOff,SIZEOF(UC),1,test);
		fclose(test);
	}
	else
	{
		fclose(test1);
	}

	if(NULL==(test1=fopen("/www/cgi-bin/morning.cfg","r")))
	{
		test = fopen("/www/cgi-bin/morning.cfg","w");
		if(test==NULL)
		{
			perror("morning.cfg test_create\n");
			exit(1);
		}
		fwrite(&timeOff,SIZEOF(UC),1,test);
		fclose(test);
	}
	else
	{
		fclose(test1);
	}

	if(NULL==(test1=fopen("/www/cgi-bin/breathe.cfg","r")))
	{
		test = fopen("/www/cgi-bin/breathe.cfg","w");
		if(test==NULL)
		{
			perror("breathe.cfg test_create\n");
			exit(1);
		}
		fwrite(&timeOff,SIZEOF(UC),1,test);
		fclose(test);
	}
	else
	{
		fclose(test1);
	}

	if(NULL==(test1=fopen("/www/cgi-bin/user.cfg","r")))		//判断定时功能是否开启文档是否存在
	{
		test = fopen("/www/cgi-bin/user.cfg","w");
		if(test==NULL)
		{
			perror("user.cfg \n");
			exit(1);
		}
		fwrite(rootP,SIZEOF(UC),strlen(rootP),test);
		fclose(test);
	}
	else
	{
		fclose(test1);
	}
}

/***************************************************
*创建线程处理一个连接
***************************************************/
void *socketProcess(void *fd)
{
	uchar getAllowed = 0;									//该连接第一次需要登录，表示是否登录成功
	uchar Recv[1024];									//接受客户端返回数据
	int judgeRecv;
	int client_fd;
	uchar state;
	client_fd = *((int*)fd);
	PRINTF("received a connection from %s \n", inet_ntoa(remote_addr.sin_addr));
	for(;;)
	{
		bzero(Recv,NUMR);
		if(-1 == recv(client_fd,Recv,NUMR,0))
		{
			perror("recv error");
			continue;
		}
PRINTF("what is getted:%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",\
		Recv[0],Recv[1],Recv[2],Recv[3],Recv[4],Recv[5],Recv[6],Recv[7],Recv[8],Recv[9],Recv[10],Recv[11],Recv[12],Recv[13],Recv[14]);
/*临时去掉用户登录操作*/
getAllowed = 1;
		if(getAllowed&&Recv[0] == 0xFE)							//表示已经登录成功,并且进入是底层命令
		{
			state = cmdProc(Recv,client_fd);
			if(0==state)break;
			else if(1==state)continue;						//执行失败重新获取数据
			else if(2==state)continue;						//执行成功等待再次接收命令
			else if(3==state)break;
		}
		else if(!getAllowed&&Recv[0]!=0x00)						//没有登录并且连接没有断开
		{
			uchar temp;
			temp=userPass(Recv);
			switch(temp)
			{
				case 0:
					getAllowed = 1;						//超级用户登录成功
					send(client_fd,successLog,strlen(successLog),0);
					break;							
				case 1:
					getAllowed = 2;						//普通用户登录成功
					send(client_fd,successLog,strlen(successLog),0);
					break;							
				case 2:
					perror("please login!");				//新建的连接必须先登录
					send(client_fd,userNotExist,strlen(userNotExist),0);
					break;
				case 3:
					perror("please login!");
					send(client_fd,passwordWrong,strlen(passwordWrong),0);
					break;
				default:
					perror("please login!");				//默认认为用户没有输入正确的用户名
					send(client_fd,userNotExist,strlen(userNotExist),0);
					break;
			}
			
		}
		else if(Recv[0]=='$'&&getAllowed)
		{
			if(!strncmp(wifiget,Recv,13))wifiProc(1,Recv,client_fd);		//wifi搜索命令
			else if((!strncmp(wifiset,Recv,13))&&Recv[strlen(Recv)-1]=='$')wifiProc(2,&Recv[13],client_fd);	//wifi设置命令,最后有个$
			else if((!strncmp(wifireset,Recv,15))&&Recv[strlen(Recv)-1]=='$')wifiProc(3,NULL,client_fd);
		}
		else if(Recv[0]!=0x00&&(1==getAllowed)&&((!strncmp(useradd,Recv,7))||(!strncmp(userdel,Recv,7))))	//只有超级用户才可以执行如下
		{
			if(!strncmp(useradd,Recv,7))userFile(Recv,1,client_fd);			//用户添加操作(修改密码就是添加一个相同的用户名，密码不一样)
			else userFile(Recv,2,client_fd);  					//用户删除
		}
		else if(Recv[0]!=0x00&&(1==getAllowed)&&((!strncmp(usersue,Recv,7))))		//用户添加用户覆盖确认
		{
			if(isCover==2)
			{
				isCover = 1;
				userFile(Recv,1,client_fd);
			}
		}
		else if(getAllowed==2&&((!strncmp(useradd,Recv,7))||(!strncmp(userdel,Recv,7))))
		{
			perror("noRightDo");
			send(client_fd,noRightDo,strlen(noRightDo),0);
		}
		else
		{
			perror("exception break");						//接连异常断开
			break;
		}

	}
	getAllowed = 0;
	free(fd);
	close(client_fd);
	pthread_exit(NULL);
}
/***************************************************
*定时刷新时间初始化函数
***************************************************/
void timeInit()
{
	signal(SIGALRM, timeUp);
	set_timer();
}
/***************************************************
*主程序
***************************************************/
int main()
{
	int sockfd,*client_fd; 				/*sock_fd监听socket；client_fd：数据传递socket */
	int ret;
	pthread_t udpID;
	
//#pragma message("i am starting the program!!!\n")
	//数据初始化
	dataInit();
	//创建需要的文件
	testCreat();
	
	//创建线程每一分钟更新一次时间
	timeInit();

	//创建线程等待搜索路由器
	ret = pthread_create(&udpID,NULL,(void*)getBroadCast,NULL);	
	if(ret !=0 )
	{
		perror("pthread_creat error broadCast\n");
		exit(1);
	}
	//开机检测一次定时
	TJudge(1);

	//用户检测
	userCheck();

	while(1)//防止建立链接是发生错误
	{
		//socket创建
		if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
		{
			perror("socket creat failed"); 
			break;
		}
		//设置套接字选项避免地址使用错误
		int on = 1;
		if((setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)))<0)
		{
			perror("setsockopt err\n");
		}
		bzero(&my_addr,sizeof(my_addr));
		my_addr.sin_family=AF_INET;
		my_addr.sin_port=htons(SERVPORT);
		my_addr.sin_addr.s_addr = INADDR_ANY;
		bzero(&(my_addr.sin_zero),8);

		if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
		{
			perror("bind  failed");
			break;
		}
		if (listen(sockfd, BACKLOG) == -1) 
		{
			perror("listen  failed");
			break;
		}
		while(1)
		{
			pthread_t thread;
			int len = sizeof(struct sockaddr_in);
			client_fd = (int *)malloc(sizeof(int));
			if ((*client_fd = accept(sockfd, (struct sockaddr *)&remote_addr, &len)) == -1) 
			{
				perror("accept failed");
				break;
			}

			if(pthread_create(&thread,NULL,socketProcess,client_fd)!=0)
			{
				perror("pthread_create error");
			}
		}
		shutdown(*client_fd,2);
		shutdown(sockfd,2);
	}
	shutdown(*client_fd,2);
	shutdown(sockfd,2);
	return 0;
}
