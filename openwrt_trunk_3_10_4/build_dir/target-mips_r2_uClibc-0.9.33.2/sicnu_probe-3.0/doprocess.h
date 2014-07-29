#ifndef __PROCESS_H
#define __PROCESS_H


/*****************************************************************
*帧头 命令 / 参数1 / 参数2 # 命令 / 参数1 / 参数2  帧尾
*****************************************************************/
struct cmdFormart{
        char *headFrames;               //服务器到ＡＰ帧头
        char *tailFrames;               //服务器到ＡＰ帧尾
        char *headFramec;               //发送到服务器
        char *tailFramec;
        char cmdSplit;                  //命令与参数之间分隔符
        char cmdGroup;                  //命令组之间分隔符
};

const struct cmdFormart cmdFormarts={
        .headFrames = "*CHTA*",
        .tailFrames = "*ATCH*",
        .headFramec = "*CHTS*",
        .tailFramec = "*STCH*",
        .cmdSplit = '/',
        .cmdGroup = ';',
};

const char *ackbusy = "*CHTS*00/-4*STCH*";
const char *getConf = "/usr/sbin/uciget.sh";
const char *commit = "uci commit";
const char *getscan = "/usr/sbin/getscan.sh";

#endif
