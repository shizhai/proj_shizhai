#include "common.h"
#include "def.h"

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

//UDP包
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define deBug 1
#if deBug
#define PRINTF(...)	{printf(__VA_ARGS__);printf("\n");}
#else
#define PRINTF(...)	{;}
#endif
					

/***************************************************
*广播，UDP广播，搜索此路由器,回发IP
***************************************************/
void* getBroadCast()
{
	int socket_udp,socket_udpr;
	struct sockaddr_in my_addr,user_addr;
	uchar bufr[9];					//接受到搜索命令
	uchar msg[9]={0xfe,0x04,0x00,0x00,0xff,0xff,0xff,0xff,0x04};	
	uchar buft[9]={0xfe,0x04,0x00,0x00,0xfe,0xfe,0xfe,0xfe,0x04};	
	int so_broadcast=1;
	socklen_t size,size1;

	/*发送地址相关设置*/
	my_addr.sin_family=AF_INET;
	my_addr.sin_port=htons(PORT_UDP);
	bzero(&(my_addr.sin_zero),8);

	/*接受端要绑定*/
	user_addr.sin_family=AF_INET;
	user_addr.sin_port=htons(PORT_UDP);
	user_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	bzero(&(user_addr.sin_zero),8);
	if((socket_udp=(socket(AF_INET,SOCK_DGRAM,0)))==-1)
	{
		perror("socket");
		exit(1);
	}
	setsockopt(socket_udp,SOL_SOCKET,SO_BROADCAST,&so_broadcast,sizeof(so_broadcast));
	if((bind(socket_udp,(struct sockaddr *)&user_addr,sizeof(struct sockaddr)))==-1) 
	{
		perror("bind");
		exit(1);
	}

	size=sizeof(user_addr);
	while(1) 
	{      
		while(1)
		{
			int judge = recvfrom(socket_udp,bufr,9,0,(struct sockaddr *)&user_addr,&size);
			my_addr.sin_addr.s_addr=inet_addr(inet_ntoa(user_addr.sin_addr));				//单播回去
			if(judge==-1)
			{	
				continue;
			}
			else
			{
				if(hexCmp(bufr,msg,9) == 0)
				{
					PRINTF("the andriod ip is:%s:what is getted :%x\n",inet_ntoa(user_addr.sin_addr),bufr[0]);
					sendto(socket_udp,buft,9,0,(struct sockaddr *)&my_addr,sizeof(my_addr));
					bzero(bufr,9);
					break;
				}
			}
		}
	}
}

/***************************************************
*比较，把十六进制数据串进行比较
***************************************************/
uchar hexCmp(uchar a[],uchar b[],uchar len)
{
	uchar i;
	for(i=0;i<len;i++)
	{
		if(a[i]!=b[i])
			return 1;
	}
	return 0;
}
/***************************************************
*拷贝，把十六进制数据串进行拷贝
***************************************************/
void hexncpy(uchar *dst,uchar *src,uint n)
{
	uchar i;
	for(i=0;i<n;i++)
	{
		dst[i] = src[i];
	}
}

/***************************************************
*校验，把收到的数据进行校验
***************************************************/
uchar XOR_Check(uchar *message)
{
	uchar i;
	uchar check = 0x00;
	for(i=1;i<=message[1]+4;i++)						//接收到上位机的数据长度可能不一样长
	{
		check ^=message[i];
	}
	return check;
}
/***************************************************
*校验，为数据产生校验位
***************************************************/
uchar Get_XOR(uchar *message,uchar j)
{
	uchar i=0;
	message[j] = 0x00;
	for(i=1;i<j;i++)
		message[j]^=message[i];
	return message[j];
}

/***************************************************
*字符串比较,
*用户传输用户名与密码过来需要解析出来用户名与密码
*用户名与密码之间使用"\n"分隔开
*返回0表示超级用户登录成功
*返回1表示登录成功
*返回2表示用户名不正确
*返回3表示密码不正确
***************************************************/
uchar userPass(uchar *login)
{
	uchar i = 0;
	uchar user[userOfLen];
	uchar password[passOfLen];
	uchar usercrrect;
	if(!strncmp(userlog,login,7))						//表示用户登录命令
	{
		for(i=8;i<strlen(login);i++)
		{
			if(login[i]==' '){PRINTF("i am in userPass\n");break;}
			user[i-8]=login[i];
		}
		user[i] = '\0';
		login = &login[i+1];
		strcpy(password,login);
PRINTF("the user is %s\n",user);
PRINTF("the password is %s\n",password);
		if((0==strcmp(user,root))&&(0==strcmp(password,rootP)))		//deamo的root密码不能修改
		{
			return 0;						//root用户成功登录
		}
		else
		{
			usercrrect = 0;
			for(i=0;i<userNumNow;i++)
			{
				if(0==strcmp(user,userStore[i].users))
				{	
					usercrrect=1;
					break;
				}
			}
			if(usercrrect)
			{
				for(i=0;i<userNumNow;i++)
				{
					if(0==strcmp(password,userStore[i].passwords))return 1;	//普通用户登录成功
					else return 3;						//用户名正确，密码不正确
				}
			}	
			else
			{
				return 2;							//用户名不正确
			}
		}
	}
	return 2;
}
/***************************************************
*用户密码文档处理函数
*如果存在覆盖就返回一个同样的
*如果添加的用户已经存在，先把isCover设置成2表示需要确认
***************************************************/
void userFile(uchar *Recv,uchar sel,int client)
{
	FILE *userFile;
	uchar i=0,j=0;
	uchar tempUser[userOfLen],tempPass[passOfLen],temRecv[userOfLen+passOfLen];
	userFile = fopen("/www/cgi-bin/user.cfg","r");
	if(NULL==userFile){perror("user.cfg can't be loaded");return;}
	fclose(userFile);
	Recv = &Recv[8];							//去掉添加用户的命令头部
	strcpy(temRecv,Recv);
	/*取出用户名判断是否已经存在该用户，如果存在回发一个确认命令*/
	for(i=0;i<strlen(Recv);i++)
	{
		if(Recv[i]==' ')break;
		tempUser[i] = Recv[i];
	}
	tempUser[i] = '\0';							//??????????????????
	strcpy(tempPass,&Recv[i+1]);						//取出密码部分(最后包括\n)
	if(1==sel)								//用户添加操作	
	{
		for(i=0;i<userNumNow;i++)
		{

			if(!strcmp(userStore[i].users,tempUser))
			{
				if(1==isCover)					//确认覆盖
				{
					uchar b = ' ';
					isCover=0;
					strcpy(userStore[i].passwords,tempPass);
					if(NULL==(userFile = fopen("/www/cgi-bin/user.cfg","w")))return;
					for(j=0;j<userNumNow;j++)
					{
						fputs(userStore[j].users,userFile);
						fputc(b,userFile);
						fputs(userStore[j].passwords,userFile);			
						fputc('\n',userFile);
					}
					fclose(userFile);
					send(client,successAdd,strlen(successAdd),0);			//发送成功添加报文
				}	
				else 
				{
					isCover = 2;						
					send(client,suerCoverUser,strlen(suerCoverUser),0);		//发送确认报文
				}
				break;
			}
		}
		if(i==userNumNow)									//添加新用户
		{
			strcpy(userStore[userNumNow].users,tempUser);
			strcpy(userStore[userNumNow].passwords,tempPass);
			userNumNow++;
			userFile = fopen("/www/cgi-bin/user.cfg","a");
			//fputs(temRecv,userFile);							//在文件尾部添加新用户
			fwrite(temRecv,sizeof(uchar),strlen(temRecv),userFile);
			send(client,successAdd,strlen(successAdd),0);					//发送成功添加报文
			fclose(userFile);
		}
	}
	else if(2==sel)											//用户删除
	{
		for(i=0;i<userNumNow;i++)
		{
			if(!strcmp(userStore[i].users,tempUser))
			{
				isCover=0;
				for(j=i;j<userNumNow-1;j++)
				{
					strcpy(userStore[j].users,userStore[j+1].users);
					strcpy(userStore[j].passwords,userStore[j+1].passwords);
				}
				userNumNow--;
				if(NULL==(userFile = fopen("/www/cgi-bin/user.cfg","w")))return;
				for(j=0;j<userNumNow;j++)
				{
					fputs(userStore[j].users,userFile);
					fputc(' ',userFile);
					fputs(userStore[j].passwords,userFile);			
					fputc('\n',userFile);
				}
				fclose(userFile);
				send(client,successDel,strlen(successDel),0);				//发送成功删除用户报文
				break;
			}
		}
	}
}


void get_mylight(uchar *get_1) 
{ 
	FILE *fp1;
	unsigned char i = 0,check = 0x00;
	if(get_1[0]==0xFE)							//判断
	{
		if(get_1[1] == 0)						//当为零的时候就删除
		{
			remove("/www/list");
		}
		else if(get_1[2]==0x00&&get_1[3]==0x00)
		{
			for(i = 1;i <=get_1[1]+4;i++)
			{
				check ^= get_1[i];
			}
			if(check == 0x00)					//校验
			{
				fp1 = fopen("/www/list","w");
				fwrite(get_1,sizeof(char),get_1[1]+5,fp1);
				fclose(fp1);
				PRINTF("the online light is:%x,%x,%x,%x,%x,%x,%x,%x\n",\
				get_1[0],get_1[1],get_1[2],get_1[3],get_1[4],get_1[5],get_1[6],get_1[7],get_1[8]);
			}
		}
		check = 0x00;
	}
}
/***************************************************
*命令处理函数
*返回0表示失败，并退出线程
*返回1表示失败，但是不退出线程
*返回2表示成功，不退出线程
*返回3表示退出，退出线程
***************************************************/
uchar cmdProc(uchar *Recv,int client_fd)
{
	int ret;
	uchar Send[NUMT];							//返回给客户端的数据
	FILE *timeCfg,*morningCfg,*breatheCfg;	
	uchar timeLight[NUMR],morningLight[NUMR],breatheLight[NUMR],*pt,*pm,*pb;
	pthread_t timeID,morningID,breatheID;					//定时,morning call,breathe线程号
	if(0==XOR_Check(Recv))
	{
		PRINTF("i am check is 0\n");
		ACK[0] = 0XFE;ACK[2] = 0X00;ACK[3] = 0X00;ACK[4] = 0XFF;	//返回命令的状态固定值
		if(Recv[4]==0xFF&&Recv[5]==0xFF)				//搜索命令
		{
			FILE *l;	
			int flen1;
			uchar serchCMD[7]={0xFE,0x02,0x00,0x00,0xFF,0xFF,0x02};
			unsigned char get_light[NUMR]={0};
			bzero(get_light,NUMR);
			bzero(Send,NUMT);
			while(1)
			{
				commanT(serchCMD,get_light);				//发送命令并读取数据
				get_mylight(get_light);
				l = fopen("/www/list","r");
				if(l==NULL)
				{
					perror("num 3\n");
					continue;
				}
				else
				{
					fseek(l,0L,SEEK_END);
					flen1 = ftell(l);
					rewind(l);
					fread(Send,SIZEOF(UC),flen1,l);		//不能读取的长度超过文件实际长度，否则读完后文件被删除
					fclose(l);
					break;
				}
			}
PRINTF("the search ff ff Send is %x,%x,%x,%x,%x,%x,%x,%x\n",Send[0],Send[1],Send[2],Send[3],Send[4],Send[5],Send[6],Send[7]);
			ret = send(client_fd,Send,Send[1]+5,0);
			if(ret == -1)
			{
				perror("i am send ERROR\n");
			}
			bzero(Send,NUMR);
		}
		else if(Recv[4]==0xFF&&Recv[5]==0xFE)						//表示结束本设备本次通讯
		{
PRINTF("bye\n");
			ACK[1] = 0X03;ACK[5] = 0XFE;
			ACK[6] = 0X01;ACK[7] = 0X03;						//FF FE命令执行成功
			send(client_fd,ACK,8,0);
			return 3;
		}
		else if((Recv[4]==0xFF)&&(Recv[5]==0x01))					//FF 01特殊命令定时配置文件
		{
			if(Recv[6]==0x00)							//灯号为00表示要删除定时
			{
				if((Recv[7]==0x10)||(Recv[7]==0x11))
				{
					if(Recv[7]==0x10)
					{
						morningOn = 0;
						morningCfg = fopen("/www/cgi-bin/morning.cfg","w");
						fclose(morningCfg);
					}
					else
					{
						breatheOn = 0;
						breatheCfg = fopen("/www/cgi-bin/breathe.cfg","w");
						fclose(breatheCfg);
					}
				}
				else
				{
					timeOn = 0;
					timeCfg = fopen("/www/cgi-bin/time.cfg","w");
					fclose(timeCfg);
				}
			}
			else if((Recv[7]==0x10)||(Recv[7]==0x11))				//定时moring call 与 breathing 配置
			{
				if(Recv[7]==0x10)						//moring call 配置文件
				{
					uchar morningOnOld = morningOn;
					morningOn = 0;						//先停止定时器
					pm =&Recv[6];
					hexncpy(morningLight,pm,Recv[1]-2);				
					morningLight[Recv[1]-2] = 0x00;
					morningCfg = fopen("/www/cgi-bin/morning.cfg","w");
					if(timeCfg!=NULL)
					{
						fwrite(morningLight,SIZEOF(UC),Recv[1]-2,morningCfg);
						fclose(morningCfg);
					}
					else
					{
						morningOn = morningOnOld;
						ACK[1] = 0X03;ACK[5] = 0X01;
						ACK[6] = 0X00;ACK[7] = 0XFD;			//FF 01命令执行失败
						send(client_fd,ACK,8,0);
						perror("open morning.cfg error");
						return 1;
					}
					morningOn = morningOnOld;
					if(morningOn)
					{
						ret = pthread_create(&morningID,NULL,(void*)morning,NULL);
						if(ret !=0 )
						{
							perror("timeID pthread_creat error");
							ACK[1] = 0X03;ACK[5] = 0X01;
							ACK[6] = 0X00;ACK[7] = 0XFD;		//FF 01命令执行失败
							send(client_fd,ACK,8,0);
							return 1;				//?????漏洞
						}
					}
					ACK[1] = 0X03;ACK[5] = 0X01;
					ACK[6] = 0X01;ACK[7] = 0XFC;				//FF 01命令执行成功
					send(client_fd,ACK,8,0);
				}
				else								//breathing的配置 
				{
					uchar breatheOnOld = breatheOn;
					breatheOn = 0;						//先停止定时器
					pb =&Recv[6];
					hexncpy(breatheLight,pb,Recv[1]-2);				
					breatheLight[Recv[1]-2] = 0x00;
					breatheCfg = fopen("/www/cgi-bin/breathe.cfg","w");
					if(breatheCfg!=NULL)
					{
						fwrite(breatheLight,SIZEOF(UC),Recv[1]-2,breatheCfg);
						fclose(breatheCfg);
					}
					else
					{
						breatheOn = breatheOnOld;
						ACK[1] = 0X03;ACK[5] = 0X01;
						ACK[6] = 0X00;ACK[7] = 0XFD;			//FF 01命令执行失败
						send(client_fd,ACK,8,0);
						perror("open breathe.cfg error");
						return 1;
					}
					breatheOn = breatheOnOld;
					if(breatheOn)
					{
						ret = pthread_create(&breatheID,NULL,(void*)breathing,NULL);
						if(ret !=0 )
						{
							perror("breatheID pthread_creat error");
							ACK[1] = 0X03;ACK[5] = 0X01;
							ACK[6] = 0X00;ACK[7] = 0XFD;		//FF 01命令执行失败
							send(client_fd,ACK,8,0);
							return 1;
						}
					}
					ACK[1] = 0X03;ACK[5] = 0X01;
					ACK[6] = 0X01;ACK[7] = 0XFC;				//FF 01命令执行成功
					send(client_fd,ACK,8,0);
				}
			}
			else									//定时配置文件
			{
				uchar timeOnOld = timeOn;
				timeOn = 0;							//先停止定时器
				pt =&Recv[6];
				hexncpy(timeLight,pt,Recv[1]-2);
				timeLight[Recv[1]-2] = 0x00;
				timeCfg = fopen("/www/cgi-bin/time.cfg","w");
				if(timeCfg!=NULL)
				{
					fwrite(timeLight,SIZEOF(UC),Recv[1]-2,timeCfg);
					fclose(timeCfg);
					ACK[1] = 0X03;ACK[5] = 0X01;
					ACK[6] = 0X01;ACK[7] = 0XFC;				//FF 01命令执行成功
					send(client_fd,ACK,8,0);
				}
				else
				{
					ACK[1] = 0X03;ACK[5] = 0X01;
					ACK[6] = 0X00;ACK[7] = 0XFD;				//FF 01命令执行失败
					send(client_fd,ACK,8,0);
					perror("open time.cfg error");
					return 1;						//执行失败，不退出线程
				}
				timeOn = timeOnOld;
				if(timeOn)
				{
PRINTF("timing go good in function \n");
					ret = pthread_create(&timeID,NULL,(void*)timming,NULL);
					if(ret !=0 )
					{
						perror("timeID pthread_creat error");
						ACK[1] = 0X03;ACK[5] = 0X01;
						ACK[6] = 0X00;ACK[7] = 0XFD;			//FF 01命令执行失败
						send(client_fd,ACK,8,0);
						return 1;					//执行失败，不退出线程
					}
				}
			}
		}
		else if((Recv[4]==0xFF)&&(Recv[5]==0x02))					//定时开启
		{	
			if(timeOn == 0)
			{
PRINTF("i am opening in funtion timing\n");
				uchar on = 0x01;
				timeOn = 0x01;
				FILE *timeCfg1;
				timeCfg1 = fopen("/www/cgi-bin/isTime","w");
				if(timeCfg1 == NULL)
				{
					perror("in the socketProcess can't open isTime");
					ACK[1] = 0X03;ACK[5] = 0X02;
					ACK[6] = 0X00;ACK[7] = 0XFE;				//FF 02命令执行失败
					send(client_fd,ACK,8,0);
					return 1;
				}
			
				fwrite(&on,SIZEOF(UC),1,timeCfg1);
				fclose(timeCfg1);
				ret = pthread_create(&timeID,NULL,(void*)timming,NULL);
				if(ret !=0 )
				{
					perror("timeID pthread_creat error");
					ACK[1] = 0X03;ACK[5] = 0X02;
					ACK[6] = 0X00;ACK[7] = 0XFE;				//FF 02命令执行失败
					send(client_fd,ACK,8,0);
					return 1;
				}
			}
			ACK[1] = 0X03;ACK[5] = 0X02;
			ACK[6] = 0X01;ACK[7] = 0XFF;						//FF 02命令执行成功
			send(client_fd,ACK,8,0);
		}
		else if((Recv[4]==0xFF)&&Recv[5]==0x03)						//关闭定时器
		{
PRINTF("i am closing timing in function\n");
			if(timeOn==1)
			{
				uchar on = 0x00;
				timeOn = 0;
				FILE *timeCfg1;
				timeCfg1 = fopen("/www/cgi-bin/isTime","w");
				if(timeCfg1==NULL)
				{
					ACK[1] = 0X03;ACK[5] = 0X03;
					ACK[6] = 0X00;ACK[7] = 0XFF;				//FF 03命令执行失败
					send(client_fd,ACK,8,0);
					perror("can't open isTime ff 03");
					return 1;
				}
				fwrite(&on,SIZEOF(UC),1,timeCfg1);
				fclose(timeCfg1);
			}
			ACK[1] = 0X03;ACK[5] = 0X03;
			ACK[6] = 0X01;ACK[7] = 0XFE;						//FF 03命令执行成功
			send(client_fd,ACK,8,0);
		}
		else if((Recv[4]==0xFF)&&(Recv[5]==0x04))					//开启morning call 
		{
			if(morningOn == 0)
			{
				morningOn = 0x01;
				uchar on = 0x01;
				FILE *morningCfg1;
				morningCfg1 = fopen("/www/cgi-bin/isMorning","w");
				if(morningCfg1 == NULL)
				{
					perror("can't open isMorning ff 04");
					ACK[1] = 0X03;ACK[5] = 0X04;
					ACK[6] = 0X00;ACK[7] = 0XFD;			//FF 04命令执行失败
					send(client_fd,ACK,8,0);
					return 1;
				}

				fwrite(&on,SIZEOF(UC),1,morningCfg1);
				fclose(morningCfg1);
				ret = pthread_create(&morningID,NULL,(void*)morning,NULL);
				if(ret !=0 )
				{
					perror("morningID pthread_creat error");
					ACK[1] = 0X03;ACK[5] = 0X04;
					ACK[6] = 0X00;ACK[7] = 0XFD;		//FF 04命令执行失败
					send(client_fd,ACK,8,0);
					return 1;
				}
			}
				ACK[1] = 0X03;ACK[5] = 0X04;
				ACK[6] = 0X01;ACK[7] = 0XFC;			//FF 04命令执行成功
			send(client_fd,ACK,8,0);
		}
		else if((Recv[4]==0xFF)&&Recv[5]==0x05)				//关闭morning call
		{
			if(morningOn==1)
			{
				uchar on = 0x00;
				morningOn = 0;
				FILE *morningCfg1;
				morningCfg1 = fopen("/www/cgi-bin/isMorning","w");
				if(morningCfg1==NULL)
				{
					ACK[1] = 0X03;ACK[5] = 0X05;	
					ACK[6] = 0X00;ACK[7] = 0XF9;		//FF 05命令执行失败
					send(client_fd,ACK,8,0);
					return 1;
				}
				fwrite(&on,SIZEOF(UC),1,morningCfg1);
				fclose(morningCfg1);
			}
			ACK[1] = 0X03;ACK[5] = 0X05;
			ACK[6] = 0X01;ACK[7] = 0XF8;				//FF 05命令执行成功
			send(client_fd,ACK,8,0);
		}
		else if((Recv[4]==0xFF)&&(Recv[5]==0x06))			//开启breathe 
		{
			if(breatheOn == 0)
			{
				uchar on = 0x01;
				breatheOn = 0x01;
				FILE *breatheCfg1;
				breatheCfg1 = fopen("/www/cgi-bin/isBreathe","w");
				if(breatheCfg1 == NULL)
				{
					perror("can't open isBreathe ff 07");
					ACK[1] = 0X03;ACK[5] = 0X06;
					ACK[6] = 0X00;ACK[7] = 0XFC;			//FF 06命令执行失败
					send(client_fd,ACK,8,0);
					return 1;
				}
			
				fwrite(&on,SIZEOF(UC),1,breatheCfg1);
				fclose(breatheCfg1);
				ret = pthread_create(&breatheID,NULL,(void*)breathing,NULL);
				if(ret !=0 )
				{
					perror("breatheID pthread_creat error");
					ACK[1] = 0X03;ACK[5] = 0X06;
					ACK[6] = 0X00;ACK[7] = 0XFC;			//FF 06命令执行失败
					send(client_fd,ACK,8,0);
					return 1;
				}
			}
			ACK[1] = 0X03;ACK[5] = 0X06;
			ACK[6] = 0X01;ACK[7] = 0XFB;				//FF 06命令执行成功
			send(client_fd,ACK,8,0);
		}
		else if((Recv[4]==0xFF)&&Recv[5]==0x07)				//关闭breathe
		{
PRINTF("i am closing breathe\n");
			if(breatheOn==1)
			{
				uchar on = 0x00;
				breatheOn = 0;
				FILE *breatheCfg1;
				breatheCfg1 = fopen("/www/cgi-bin/isBreathe","w");
				if(breatheCfg1==NULL)
				{
					ACK[1] = 0X03;ACK[5] = 0X07;
					ACK[6] = 0X00;ACK[7] = 0XFB;		//FF 07命令执行失败
					send(client_fd,ACK,8,0);
					return 1;
				}
				fwrite(&on,SIZEOF(UC),1,breatheCfg1);
				fclose(breatheCfg1);
			}
			ACK[1] = 0X03;ACK[5] = 0X07;
			ACK[6] = 0X01;ACK[7] = 0XFA;			//FF 07命令执行成功
			send(client_fd,ACK,8,0);
		}
		else
		{
PRINTF("i am cmd\n");
			uchar get_ack[NUMR];
			commanT(Recv,get_ack);		
			#if deBug		
			send(client_fd,Recv,Recv[1]+5,0);			
			#else
			send(client_fd,get_ack,get_ack[1]+5,0);
			#endif
			
		}
		bzero(Recv,NUMR);
	}
	else
	{
		return 1;								//校验失败，不退出线程
	}
	return 2;									//成功
}













