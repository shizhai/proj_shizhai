#ifndef _common_h_
#define _common_h_

#include "def.h"

void hexncpy(uchar *dst,uchar *src,uint n);
uchar hexCmp(uchar *a,uchar *b,uchar len);
void commanT(uchar *Re,uchar *ack);
uchar XOR_Check(uchar *message);
void* getBroadCast();
void timeproc(uchar *timeSet,uint len); 
void timeproc1(uchar *timeSet,uint len);
void timeproc2(uchar *timeSet,uint len);
//void morningProc(uchar *temp);
//void breatheProc(uchar *temp);
void* timming();
void* morning();
void* breathing();
void userFile(uchar *Recv,uchar sel,int client);
void userCheck();
void set_timer();
void timeUp();
//int  uartInit (const char *dev_name, int nSpeed , int  nBits , char nEvent , int  nStop );

#endif
