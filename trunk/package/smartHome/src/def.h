#ifndef __def_h_
#define __def_h_

typedef unsigned char uchar;
typedef unsigned int uint;

#define SIZEOF(UC)	sizeof(unsigned char)
#define PORT_UDP 6001				/*UDP端口号 */
#define SERVPORT 6000 				/*服务器监听端口号*/
#define BACKLOG 10 				/* 最大同时链接请求数 */

#define NUMR	108
#define NUMT	108
#define MAXL	255
#define MAXDATASIZE	128			/*UDP包最大容量*/
#define MAXUSERS	20			/*最多的使用用户数量,但是同时使用的人数为上面定义的最大同时连接数目BACKLOG*/

#define lightNum 100				/*可以为100盏灯设置*/
#define lightProc 24				/*对于一个灯最多可以有24个时间段的设置*/

#define userOfLen	64
#define passOfLen	64


uchar ACK[NUMT];				/*特殊命令的下位机反馈信息*/

uchar timeGet[2];

/*--15----14----13----12----11----10----9----8----7----6----5----4----3------------2------------1---------------0-------*/
/*                                                                              breathe      morning call      定时
/*相应位为1表示该功能启动，否则该功能没有启动	
/*----------------------------------------------------------------------------------------------------------------------*/
uint isOn;					/*根据位来判断相应的功能是否开启*/
//static uint TIMEON=0x0001;
//static uint MORNINGON=0x0002;
//static uint BREATHEON=0x0004;
uchar timeOn;					/*定时是否开启(初始否)*/
uchar morningOn;				/*morning call 是否开启(初始否)*/
uchar breatheOn;				/*呼吸灯是否开启(初始否)*/

uchar userNumNow;				/*目前的用户数量*/

struct allusers{
	char users[userOfLen];
	char passwords[passOfLen];

}userStore[MAXUSERS];

uchar isCover;					/*用户覆盖*/
static uchar timeIsUpdate;			/*为1时时间已经执行一次更新*/
static uchar timeIsUpdate1;
static uchar timeIsUpdate2;			


/*上层命令的回复与确认命令*/
static char *successLog="ACK:SUCCESSLOG";
static char *relogin="ACK:RELOGIN";
static char *passwordWrong="ACK:PASSWORDWRONG";			//登录失败密码错误时候返回
static char *userNotExist="ACK:USERNOTEXIST";			//登录失败用户名不存在时候返回
static char *noRightDo="ACK:NORIGHTDO";				//没有权限执行
static char *suerCoverUser="ACK:SUERCOVERUSER";
static char *successAdd="ACK:SUCCESSADD";
static char *successDel="ACK:SUCCESSDEL";

static char *usersue="usersue";					//用户确认覆盖命令
static char *userlog="userlog";					//用户登录命令头部
static char *useradd="useradd";					//用户添加命令头部
static char *userdel="userdel";					//用户删除命令头部

static char *root="root";
static char *rootP="123456";

static char *wifiget="$wifiget now$";				//wifi搜索命令
static char *wifiset="$wifiset now$";				//wifi加入命令$wifiset now$macid_-?+?-_openwrtAP_-?+?-_key$
static char *wifireset="$wifireset now$";			//把wifi模式恢复到AP模式 
static char *wifisetPart="_-?+?-_";
static char *successjoinin="ACK:SUCCESSJOIN";
static char *failtojoinin="ACK:FAILTOJOININ";
static char *successwifireset="ACK:SUCCESSWIFIRESET";


#endif
