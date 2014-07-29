#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>

#include <sys/socket.h>		//struct sockaddr , listen, accept, connect , send, recieve...
#include <netinet/in.h>		//struct sockaddr_in
#include <arpa/inet.h>		//inet_addr inet_ntoa...

#include "doprocess.h"
#include "dosem.h"
#include "doprocess1.h"
#include "main.h"

char isBusy = 0;

extern const char confscan[];
extern const char confmcs[];
void sig_proccess(int signo)
{
	/*子进程退出也会打印该信息*/
	PRINTF("Catch a exit signal and exit stations_collect\n");
	exit(0);	
}

void sig_pipe(int sign)
{
	PRINTF("Catch a SIGPIPE signal\n");
}

/****************************************命令接收主进程*************************************************/
/*命令解析函数*/
int cmd_process(int s)
{
	FILE *fwconf;
	switch(cmds.state)
	{
	case '5':
		/*配置扫描*/
		fwconf = fopen(confscan, "w");
		fputs(cmds.scan_t.cifnamex, fwconf);			//扫描接口写入配置文件　
		fputs(cmds.scan_t.csip, fwconf);			//扫描上传服务器IP写入文件　
		fputs(cmds.scan_t.ccmd, fwconf);			//扫描的信道参数写入配置文件
		fputs(cmds.scan_t.cpara, fwconf);
		fclose(fwconf);
		strncmp(cmds.scan_t.cifnamex, "none", 5)?do_conf(1, MASKSCAN):do_conf(2, MASKSCAN);	//配置扫描，并且关闭子进程
		break;
	case '6':
		/*配置无线网络*/
		fwconf = fopen(confmcs, "w");
		fputs(cmds.cap_t.cifnamex, fwconf);
		fputs(cmds.cap_t.cmcs, fwconf);
		fclose(fwconf);
		strncmp(cmds.scan_t.cifnamex, "none", 5)?do_conf(1, MASKCAP):do_conf(2, MASKCAP);
		break;
	case '7':
		/*配置有线网络*/
		strncmp(cmds.scan_t.cifnamex, "none", 5)?do_conf(1, MASKCNET):do_conf(2, MASKCNET);
		break;
	case '8':
		/*系统重启命令*/
		system("reboot");
		break;
	case '9':
		/*查询当前配置*/

		break;
	case 'b':
		/*查询下属设备*/
		/*返回值：*CHTS*MAC RSSI/MAC RSSI...*STCH**/
		break;
	case 'c':
		/*cmdparam中MAC地址踢下线*/
		break;
	default:
		break;	
	}
	return 1;
}
/*命令接收进程*/
void process_conn_server(int s)
{
        ssize_t size = 0;
	char buffer[CMDLEN], *pbuffer;
	pbuffer = buffer;
	PRINTF("the test in process_conn_server!\n");
	/*等待接收命令*/
	size = read(s, buffer, CMDLEN);                 //接收命令数据包
	if(size > 0)
	{
		if(isBusy)
		{
			/*发送忙信号*/
			write(s, ackbusy, strlen(ackbusy));
		}
		else if((0==strncmp(buffer, cmdFormarts.headFrames, strlen(cmdFormarts.headFrames))) \
				&& (0 == strcmp(&buffer[strlen(buffer)-6], cmdFormarts.tailFrames)))
		{
			isBusy = 1;
			/*去掉帧头与帧尾*/
			buffer[strlen(pbuffer)-6] = '\0';
			pbuffer += 6;
			PRINTF("the cmd line is %s \n",pbuffer);
			/*分组命令分别进行执行*/
			memcpy(&cmds, (struct cmd *)pbuffer, strlen(pbuffer));
			cmd_process(s);
			isBusy = 0;
		}
	}
	else return;
}

/****************************************上传子进程*************************************************/

/*获取获取到的数据，读取到缓冲区*/
uint8_t scan24_5(const char* pathx, char *buffer, char *split)
{
	FILE *fstations;
	char tmpc[32];
	tmpc[0] = 0x00;
	fstations = fopen(pathx, "r");
	if(fstations==NULL)
	{ 
		PRINTF("fstations for horst!\n");
		return 0;
	}
	while(NULL != fgets(tmpc, 32, fstations))
	{
		strcat(buffer, tmpc);
		/*去掉回车符*/
		buffer[strlen(buffer)-1] = '\0';
		/*追加分隔符*/
		strcat(buffer, split);
	}
	fclose(fstations);
	//PRINTF("return 1 in scan24_5 the buffer is:%s\n", buffer);
	return 1;
}
/*获取数据，发送到ＵＤＰ服务器,优化把例如strlen之类的多处调用使用先调用赋值给变量，再使用变量*/
void stations_collect()
{
	signal(SIGINT, sig_proccess);
	/*扫描结果数据结构体*/
	//struct buf_devic buffer_device;	

	struct sockaddr_in to;
	char buffer[SCANNUM];
	int sudp;
	uint8_t isconn = 0;

	PRINTF("test comein stations_collect and udp server ip: %s\n", cmds.scan_t.csip);

	sudp = socket(AF_INET, SOCK_DGRAM, 0); 
	if( sudp == -1 )
	{
		perror("socket udp");
		exit(EXIT_FAILURE);
	}
	memset(&to, 0, sizeof(to));
	to.sin_family = AF_INET;
	to.sin_addr.s_addr = inet_addr(cmds.scan_t.csip);
	to.sin_port = htons(SCA_PORT);

	for( ; ; )
	{
		isconn = 0;
		buffer[0] = '\0';
		/*添加回复报文帧头*/
		strcpy(buffer, cmdFormarts.headFramec);	
		/*等待扫描程序使能*/
		sem_v(scan_semid);
		/*产生扫描到的设备信息及rssi*/
		system(getscan);
		isconn = scan24_5("/tmp/pesGet24", buffer, "/");
		if(0 == isconn)		//打开文件失败，重新尝试生成
		{
			sem_p(main_semid);
			PRINTF("error open file in stations_collect!\n");
			continue;	
		}
		buffer[strlen(buffer)-1] = '\0';
		strcat(buffer, cmdFormarts.tailFramec);
		/*扫描程序继续活动*/
		sem_p(main_semid);
		//PRINTF("the send is:%s\n", buffer);
		sendto(sudp, buffer, strlen(buffer), 0, (struct sockaddr *)&to, sizeof(to));
	}
	return ;	
}

