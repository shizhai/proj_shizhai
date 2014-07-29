#ifndef _wifi_h_
#define _wifi_h_

/*------------------------------------------------------------------------------------------------*/
/*==============================================wifi.c============================================*/
/*------------------------------------------------------------------------------------------------*/

#define WIFI_ID_MAX	64
#define WIFI_NAME_MAX	255

static const char signals[7]="signal:";
static const char SSIDs[5]="SSID:";
static const char optionid[11]="option ssid";
static const char optionkey[10]="option key";
static const char MACID[3]="BSS";

void wifiProc(unsigned char sel,unsigned char arg[],int tcp_fd);

#endif

