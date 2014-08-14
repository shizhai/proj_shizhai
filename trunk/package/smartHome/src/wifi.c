#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <fcntl.h>

#include "wifi.h"
#include "def.h"


#define deBug 1
#if deBug
#define PRINTF(...)	{printf(__VA_ARGS__);printf("\n");}
#else
#define PRINTF(...)	{;}
#endif

unsigned char wifiDevNums;
char wifi[65535];

char *str(char *p)
{
	unsigned char i;
	while(*p++==0x20);
	p--;
	while(*p++==0x09);
	p--;
	i = strlen(p);
	while(p[--i]==0x0a);
	p[++i] = '\0';
	return p;
}
void mysprintf(char *temp)
{
	char i=0;
	char tmp[255];
	sprintf(&tmp[1],"%s",temp);
	tmp[0]='\'';
	i=strlen(tmp);
	tmp[i]='\'';
	tmp[i+1]=0x00;
	sprintf(temp,"%s",tmp);
}
char *str1(char *p)
{
	unsigned char i;
	while(*p++==0x20);
	p--;
	while(*p++==0x09);
	p--;
	return p;
}
/***************************************************
*wifi数据初始化 
***************************************************/
void wifiInit()
{
	wifiDevNums = 0;
	bzero(wifi,sizeof(wifi));
}
/***************************************************
*wifi加入用户名与密码分解 openwrt_-?+?-_123456$
***************************************************/
void wifiDepart(uchar get[],char *name,char *key,char *macid)
{
	uchar position=0;
	char *tmp;
	position = strlen(get)-strlen(strstr(get,wifisetPart));
	strncpy(macid,get,position);			//获取mac地址	
	get=&get[position+strlen(wifisetPart)];
	position = strlen(get)-strlen(strstr(get,wifisetPart));
	strncpy(name,get,position);
	tmp=&get[position+strlen(wifisetPart)];
	tmp[strlen(tmp)-1]=0x00;			//去掉'$'
	strcpy(key,tmp);
	return;
}
/***************************************************
*wifi搜索与加入处理
***************************************************/
void wifiProc(unsigned char sel,unsigned char arg[],int tcp_fd)
{
	FILE *stream;
	unsigned char i=0,j=0;
	wifiInit();
	if(1==sel)								//wifi 搜索命令
	{
		char buf[128];
		char signalSSIDs[255];
		system("iw dev wlan0 scan>/www/cgi-bin/wifi_list");
		stream = fopen("/www/cgi-bin/wifi_list","r");
		while(!feof(stream))
		{
			bzero(buf,sizeof(buf));
			fgets(buf,128,stream);
			sprintf(buf,"%s",str(buf));
			if(!strncmp(MACID,buf,3))
			{
				i=0;
				bzero(signalSSIDs,sizeof(signalSSIDs));
				sprintf(buf,"%.128s",&buf[4]);
				strncpy(signalSSIDs,buf,17);
				i++;
			}
			if(!strncmp(signals,buf,7))
			{
				j = strlen(buf);
				buf[j] = ' ';
				buf[j+1] = '\0';
				strcat(signalSSIDs,buf);
				if(i!=1)
				{
					bzero(signalSSIDs,sizeof(signalSSIDs));
					i=0;
					continue;
				}
				else i++;
			}
			if(!strncmp(SSIDs,buf,5))
			{
				if(strlen(buf)<=6){i--;continue;}
				strcat(signalSSIDs,buf);
				if(i!=2)
				{
					bzero(signalSSIDs,sizeof(signalSSIDs));
					i=0;
					continue;
				}
				else i++;
			}
			if(i==3)
			{
				i=0;
				wifiDevNums++;
				strcat(wifi,signalSSIDs);
				strcat(wifi,"\n");
			}
		}
		fclose(stream);
		stream=NULL;
		send(tcp_fd,wifi,strlen(wifi),0);				//发送wifi搜索结果报文
		/*测试*/
		PRINTF("%s\n",wifi);
	}
	else if(2==sel)								//wifi加入处理
	{
		char name[255],key[255],bssid[32];
		char uciSet[512];						//uci设置命令缓存
		char uciGet[128];

		//system("ifconfig wlan0 down");
		system("cp /etc/config/wirelessb /etc/config/wireless");	//wirelesso,networko为作为ap 时的配置
		system("cp /etc/config/networkb /etc/config/network");		//wirelessb,networkb为作为sta时的配置
		bzero(name,sizeof(name));
		bzero(key,sizeof(key));
		wifiDepart(arg,name,key,bssid);
		mysprintf(name);
		mysprintf(key);	
		mysprintf(bssid);
		//checkName(name);						//检测name是否存在ssid列表中
		PRINTF("the name is %s,the key is %s,the bssid is %s\n",name,key,bssid);		//测试name与key获取是否正确
		sprintf(uciSet,"%.128s%.255s","uci set wireless.@wifi-iface[0].ssid=",name);			
		PRINTF("the uciSet of name set is %s\n",uciSet);		//测试命令组合是否正确			
		system(uciSet);
		bzero(uciSet,strlen(uciSet));
		sprintf(uciSet,"%.128s%.255s","uci set wireless.@wifi-iface[0].key=",key);
		PRINTF("the uciSet of key is %s\n",uciSet);	
		system(uciSet);
		bzero(uciSet,strlen(uciSet));
		sprintf(uciSet,"%.128s%.255s","uci set wireless.@wifi-iface[0].bssid=",bssid);
		PRINTF("the uciSet of bssid is %s\n",uciSet);	
		system(uciSet);
		system("uci commit wireless");
		send(tcp_fd,successjoinin,strlen(successjoinin),0);		//发送wifi加入操作成功，是否加入成功未作判断
		system("/etc/init.d/network restart");
		system("ifconfig wlan0 up");
		sleep(1);
	}
	else if(3==sel)
	{
		system("cp /etc/config/wirelesso /etc/config/wireless");	//wirelesso,networko为作为ap 时的配置
		system("cp /etc/config/networko /etc/config/network");		//wirelessb,networkb为作为sta时的配置
		send(tcp_fd,successwifireset,strlen(successwifireset),0);
		system("ifconfig wlan0 up");
		system("/etc/init.d/network restart");
	}
}

