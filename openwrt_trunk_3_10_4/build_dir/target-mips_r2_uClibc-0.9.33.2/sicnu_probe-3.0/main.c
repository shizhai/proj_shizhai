/* horst - Highly Optimized Radio Scanning Tool
 *
 * Copyright (C) 2005-2011 Bruno Randolf (br1@einfach.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <err.h>

#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <unistd.h>		//fork头文件　

#include <sys/socket.h>		//struct sockaddr , listen, accept, connect , send, recieve...
#include <netinet/in.h>		//struct sockaddr_in
#include <arpa/inet.h>		//inet_addr inet_ntoa...

#include "main.h"
#include "util.h"
#include "capture.h"
#include "protocol_parser.h"
#include "network.h"
#include "display.h"
#include "ieee80211.h"
#include "ieee80211_util.h"
#include "wext.h"


#include "dosem.h"
#include "doprocess1.h"

static struct itimerval oldtv;

int scan_semid;
int main_semid;
key_t scan_key;
key_t main_key;

struct list_head nodes;
struct essid_meta_info essids;
struct history hist;
struct statistics stats;
struct chan_freq channels[MAX_CHANNELS];
struct channel_info spectrum[MAX_CHANNELS];

struct config conf = {
	.node_timeout		= NODE_TIMEOUT,
	.channel_time		= CHANNEL_TIME,
	.ifname			= INTERFACE_NAME,
	.display_interval	= DISPLAY_UPDATE_INTERVAL,
	.filter_pkt		= 0xffffff,
	.recv_buffer_size	= RECV_BUFFER_SIZE,
	.port			= DEFAULT_PORT,
};

struct cmd cmds;

struct timeval the_time;

int mon; /* monitoring socket */

pid_t uppid;						/*上传PID*/
pid_t scanpid;						/*扫描执行PID*/
char isPidUp=0;						/*上传进程是否存在*/
char isPidScan=0;					/*扫描进程是否存在*/

/*输出结果到临时文件*/
char *outf="/tmp/allget";
static FILE* DF = NULL;

/*开机动态配置文件，只有scan里面的接口，ap里面的mcs需要真实动态配置*/
const char *confscan = "/etc/cscan";
const char *confmcs = "/etc/cmcs";
const char *getReayFiles = "/usr/sbin/readyFiles.sh";
const char *monitor = "/usr/sbin/monitor.sh ";
const char *station = "/usr/sbin/station.sh ";
const char *setap = "/usr/sbin/ap.sh ";
const char *setnet = "/usr/sbin/net.sh ";

int cmd_ss,cmd_sc;						/* ss为服务器的socket描述符，sc为客户端的socket描述符*/
struct sockaddr_in cmd_server_addr;				/* 服务器地址结构*/
struct sockaddr_in cmd_client_addr;				/*客户端地址结构*/

/* receive packet buffer
 *
 * due to the way we receive packets the network (TCP connection) we have to
 * expect the reception of partial packet as well as the reception of several
 * packets at one. thus we implement a buffered receive where partially received
 * data stays in the buffer.
 *
 * we need two buffers: one for packet capture or receiving from the server and
 * another one for data the clients sends to the server.
 *
 * not sure if this is also an issue with local packet capture, but it is not
 * implemented there.
 *
 * size: max 80211 frame (2312) + space for prism2 header (144)
 * or radiotap header (usually only 26) + some extra */
static unsigned char buffer[2312 + 200];
static size_t buflen;

/* for packets from client to server */
static unsigned char cli_buffer[500];
static size_t cli_buflen;

/* for select */
static fd_set read_fds;
static fd_set write_fds;
static fd_set excpt_fds;
static struct timeval tv;


struct node_info* node_update(struct packet_info* p);
void update_essids(struct packet_info* p, struct node_info* n);
void timeout_nodes(void);
int auto_change_channel(int mon);
void get_current_channel(int mon);


void __attribute__ ((format (printf, 1, 2)))
printlog(const char *fmt, ...)
{
	char buf[128];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(&buf[1], 127, fmt, ap);
	va_end(ap);

	if (conf.quiet || DO_DEBUG || !conf.display_initialized)
		printf("%s\n", &buf[1]);
	else {
		/* fix up string for display log */
		buf[0] = '\n';
		display_log(buf);
	}
}


static void
update_history(struct packet_info* p)
{
	if (p->phy_signal == 0)
		return;

	hist.signal[hist.index] = p->phy_signal;
	hist.noise[hist.index] = p->phy_noise;
	hist.rate[hist.index] = p->phy_rate;
	hist.type[hist.index] = p->wlan_type;
	hist.retry[hist.index] = p->wlan_retry;

	hist.index++;
	if (hist.index == MAX_HISTORY)
		hist.index = 0;
}


static void
update_statistics(struct packet_info* p)
{
	if (p->phy_rate == 0)
		return;

	stats.packets++;
	stats.bytes += p->wlan_len;						//总共收到数据包的长度
	if (p->wlan_retry)
		stats.retries++;

	if (p->phy_rate > 0 && p->phy_rate < MAX_RATES) {
		stats.duration += p->pkt_duration;				//到达数据包的持续更新,即是否有持续的数据到达
		stats.packets_per_rate[p->phy_rate]++;				//每个信道的设备个数更新
		stats.bytes_per_rate[p->phy_rate] += p->wlan_len;		//每个信道收到的数据包长度更新
		stats.duration_per_rate[p->phy_rate] += p->pkt_duration;	//每个信道到达数据包的持续更新
	}
	if (p->wlan_type >= 0 && p->wlan_type < MAX_FSTYPE) {
		stats.packets_per_type[p->wlan_type]++;				//对于数据包的类型更新
		stats.bytes_per_type[p->wlan_type] += p->wlan_len;
		if (p->phy_rate > 0 && p->phy_rate < MAX_RATES)
			stats.duration_per_type[p->wlan_type] += p->pkt_duration;
	}
}


static void
update_spectrum(struct packet_info* p, struct node_info* n)
{
	struct channel_info* chan;
	struct chan_node* cn;

	if (p->pkt_chan_idx < 0)
		return; /* chan not found */

	chan = &spectrum[p->pkt_chan_idx];
	chan->signal = p->phy_signal;
	chan->noise = p->phy_noise;
	chan->packets++;
	chan->bytes += p->wlan_len;
	chan->durations += p->pkt_duration;
	ewma_add(&chan->signal_avg, -chan->signal);

	if (!n)
		return;

	/* add node to channel if not already there */
	list_for_each_entry(cn, &chan->nodes, chan_list) {
		if (cn->node == n) {
			DEBUG("SPEC node found %p\n", cn->node);
			break;
		}
	}
	if (cn->node != n) {
		DEBUG("SPEC node adding %p\n", n);
		cn = malloc(sizeof(struct chan_node));
		cn->node = n;
		cn->chan = chan;
		ewma_init(&cn->sig_avg, 1024, 8);
		list_add_tail(&cn->chan_list, &chan->nodes);
		list_add_tail(&cn->node_list, &n->on_channels);
		chan->num_nodes++;
		n->num_on_channels++;
	}
	/* keep signal of this node as seen on this channel */
	cn->sig = p->phy_signal;
	ewma_add(&cn->sig_avg, -cn->sig);
	cn->packets++;
}


void
update_spectrum_durations(void)
{
	/* also if channel was not changed, keep stats only for every channel_time.
	 * display code uses durations_last to get a more stable view */
	if (conf.current_channel >= 0) {
		spectrum[conf.current_channel].durations_last =
				spectrum[conf.current_channel].durations;
		spectrum[conf.current_channel].durations = 0;
		ewma_add(&spectrum[conf.current_channel].durations_avg,
			 spectrum[conf.current_channel].durations_last);
	}
}


static void 
write_to_file(struct packet_info* p)
{
	fprintf(DF, "%s, %s, ",
		get_packet_type_name(p->wlan_type), ether_sprintf(p->wlan_src));
	fprintf(DF, "%s, ", ether_sprintf(p->wlan_dst));
	fprintf(DF, "%s, ", ether_sprintf(p->wlan_bssid));
	fprintf(DF, "%x, %d, %d, %d, %d, %d, ",
		p->pkt_types, p->phy_signal, p->phy_noise, p->phy_snr,
		p->wlan_len, p->phy_rate);
	fprintf(DF, "%016llx, ", (unsigned long long)p->wlan_tsf);
	fprintf(DF, "%s, %d, %d, %d, ",
		p->wlan_essid, p->wlan_mode, p->wlan_channel, p->wlan_wep);
	fprintf(DF, "%s, ", ip_sprintf(p->ip_src));
	fprintf(DF, "%s, ", ip_sprintf(p->ip_dst));
	fprintf(DF, "%d, %d, %d\n", p->olsr_type, p->olsr_neigh, p->olsr_tc);
}


static int
filter_packet(struct packet_info* p)
{
	int i;

	if (conf.filter_off)
		return 0;

	if (!(p->pkt_types & conf.filter_pkt)) {
		stats.filtered_packets++;
		return 1;
	}

	if (MAC_NOT_EMPTY(conf.filterbssid) &&
	    memcmp(p->wlan_bssid, conf.filterbssid, MAC_LEN) != 0) {
		stats.filtered_packets++;
		return 1;
	}

	if (conf.do_macfilter) {
		for (i = 0; i < MAX_FILTERMAC; i++) {
			if (MAC_NOT_EMPTY(p->wlan_src) &&
			    conf.filtermac_enabled[i] &&
			    memcmp(p->wlan_src, conf.filtermac[i], MAC_LEN) == 0) {
				return 0;
			}
		}
		stats.filtered_packets++;
		return 1;
	}
	return 0;
}


void
handle_packet(struct packet_info* p)
{
	struct node_info* n;
	int i = -1;

	/* filter on server side only */
	if (!conf.serveraddr && filter_packet(p)) {
		if (!conf.quiet && !conf.paused)
			update_display_clock();
		return;
	}

	if (cli_fd != -1)
		net_send_packet(p);

	if (conf.dumpfile != NULL)
		write_to_file(p);

	if (conf.quiet || conf.paused)
		return;

	/* get channel index for packet */
	if (p->phy_chan) {
		/* find channel index from packet channel */
		for (i = 0; i < conf.num_channels && i < MAX_CHANNELS; i++)
			if (channels[i].chan == p->phy_chan)
				break;
	}
	/* not found from pkt, best guess from config but it might be
	 * unknown (-1) too */
	if (i < 0 || i >= conf.num_channels || i >= MAX_CHANNELS)
		p->pkt_chan_idx = conf.current_channel;
	else
		p->pkt_chan_idx = i;

	/* detect if noise reading is present or not */
	if (!conf.have_noise && p->phy_noise)
		conf.have_noise = 1;

	/* if current channel is unknown (this is a mac80211 bug), guess it from
	 * the packet */
	if (conf.current_channel < 0 && p->pkt_chan_idx >= 0)
		conf.current_channel = p->pkt_chan_idx;

	n = node_update(p);

	if (n)
		p->wlan_retries = n->wlan_retries_last;

	p->pkt_duration = ieee80211_frame_duration(
				p->phy_flags & PHY_FLAG_MODE_MASK,
				p->wlan_len, p->phy_rate * 5,
				p->phy_flags & PHY_FLAG_SHORTPRE,
				0 /*shortslot*/, p->wlan_type, p->wlan_qos_class,
				p->wlan_retries);

	update_history(p);
	update_statistics(p);
	update_spectrum(p, n);
	update_essids(p, n);

#if !DO_DEBUG
	update_display(p, n);
#endif
}


static void
local_receive_packet(int fd, unsigned char* buffer, size_t bufsize)
{
	int len;
	struct packet_info p;

	len = recv_packet(fd, buffer, bufsize);

#if DO_DEBUG
	dump_packet(buffer, len);
#endif
	memset(&p, 0, sizeof(p));

	if (!parse_packet(buffer, len, &p)) {
		DEBUG("parsing failed\n");
		return;
	}

	handle_packet(&p);
}


static void
receive_any(void)
{
	int ret, mfd;

	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	FD_ZERO(&excpt_fds);
	//FD_SET(0, &read_fds);
	FD_SET(mon, &read_fds);
	if (srv_fd != -1)
		FD_SET(srv_fd, &read_fds);
	if (cli_fd != -1)
		FD_SET(cli_fd, &read_fds);

	tv.tv_sec = 0;
	tv.tv_usec = min(conf.channel_time, 1000000);
	mfd = max(mon, srv_fd);
	mfd = max(mfd, cli_fd) + 1;

	ret = select(mfd, &read_fds, &write_fds, &excpt_fds, &tv);
	if (ret == -1 && errno == EINTR) /* interrupted */
		return;
	if (ret == 0) { /* timeout */
		if (!conf.quiet && !DO_DEBUG)
			update_display_clock();
		return;
	}
	else if (ret < 0) /* error */
		err(1, "select()");

	/* stdin */
	//if (FD_ISSET(0, &read_fds) && !conf.quiet)
	//	handle_user_input();

	/* local packet or client */
	if (FD_ISSET(mon, &read_fds)) {
		if (conf.serveraddr)
			net_receive(mon, buffer, &buflen, sizeof(buffer));
		else
			local_receive_packet(mon, buffer, sizeof(buffer));
	}

	/* server */
	if (srv_fd > -1 && FD_ISSET(srv_fd, &read_fds))
		net_handle_server_conn();

	/* from client to server */
	if (cli_fd > -1 && FD_ISSET(cli_fd, &read_fds))
		net_receive(cli_fd, cli_buffer, &cli_buflen, sizeof(cli_buffer));
}


void
free_lists(void)
{
	int i;
	struct essid_info *e, *f;
	struct node_info *ni, *mi;
	struct chan_node *cn, *cn2;

	/* free node list */
	list_for_each_entry_safe(ni, mi, &nodes, list) {
		DEBUG("free node %s\n", ether_sprintf(ni->last_pkt.wlan_src));
		list_del(&ni->list);
		free(ni);
	}

	/* free essids */
	list_for_each_entry_safe(e, f, &essids.list, list) {
		DEBUG("free essid '%s'\n", e->essid);
		list_del(&e->list);
		free(e);
	}

	/* free channel nodes */
	for (i = 0; i < conf.num_channels; i++) {
		list_for_each_entry_safe(cn, cn2, &spectrum[i].nodes, chan_list) {
			DEBUG("free chan_node %p\n", cn);
			list_del(&cn->chan_list);
			cn->chan->num_nodes--;
			free(cn);
		}
	}
}


static void
finish_all(void)
{
	free_lists();

	if (!conf.serveraddr)
		close_packet_socket(mon, conf.ifname);

	if (DF != NULL)
		fclose(DF);

#if !DO_DEBUG
	net_finish();

	if (!conf.quiet)
		finish_display();
#endif
}


static void
exit_handler(void)
{
	/*首先退出两个子进程*/
	kill(scanpid, SIGINT);
	kill(uppid, SIGINT);
	finish_all();
}


static void
sigint_handler(int sig)
{
	/*首先退出两个子进程*/
	kill(scanpid, SIGINT);
	kill(uppid, SIGINT);
	/*主进程退出*/
	exit(0);
}


static void
sigpipe_handler(int sig)
{
	/* ignore signal here - we will handle it after write failed */
}

/*定时器初始化*/
void set_timer()
{
	struct itimerval itv;
	itv.it_interval.tv_sec = 3; 
	itv.it_interval.tv_usec = 0;
	itv.it_value.tv_sec = 1;
	itv.it_value.tv_usec = 0;
	setitimer(0, &itv, &oldtv);
	//setitimer(ITIMER_REAL, &itv, NULL);
}

/*定时操作*/
void timeUp()
{
	fclose(DF);
	/*告之主程序可以读取*/
	sem_p(scan_semid);
	/*等待主程序读取完毕，初始化为已经读取*/
	sem_v(main_semid);
	/*清空缓存区*/
	DF = fopen(conf.dumpfile, "w");
}

/*扫描进程*/
int do_scan(void)
{
	char *ifa="mon0";
	INIT_LIST_HEAD(&essids.list);
	INIT_LIST_HEAD(&nodes);

	PRINTF("test come in do_scan\n");

	/*系统信号抓取*/
	signal(SIGINT, sigint_handler);
	signal(SIGPIPE, sigpipe_handler);

	conf.ifname = ifa;					//扫描接口
	conf.dumpfile = outf;					//输出结果临时文件
	conf.quiet=1;						//不显示控制界面

	gettimeofday(&stats.stats_time, NULL);
	gettimeofday(&the_time, NULL);

	conf.current_channel = -1;

	mon = open_packet_socket(conf.ifname, sizeof(buffer), conf.recv_buffer_size);
	if (mon <= 0)
		err(1, "Couldn't open packet socket");

	conf.arphrd = device_get_arptype(mon, conf.ifname);
	if (conf.arphrd != ARPHRD_IEEE80211_PRISM &&
			conf.arphrd != ARPHRD_IEEE80211_RADIOTAP) {
		printf("Wrong monitor type! Please use radiotap or prism2 headers\n");
		exit(1);
	}

	/* get available channels */
	conf.num_channels = wext_get_channels(mon, conf.ifname, channels);
	printf("the conf.num_channels is: %d\n", conf.num_channels);
	conf.num_channels = 11;
	init_channels();
	get_current_channel(mon);

	if (!conf.quiet && !DO_DEBUG)
		init_display();

	if (conf.dumpfile != NULL) {
		DF = fopen(conf.dumpfile, "w");
		if (DF == NULL)
			err(1, "Couldn't open dump file");
	}

	/*定时器相关*/
	/*每隔３秒清空文件一次*/
        signal(SIGALRM, timeUp);
        set_timer();

	for ( /* ever */ ;;)
	{
		receive_any();
		gettimeofday(&the_time, NULL);
		timeout_nodes();
		if (!conf.serveraddr) { /* server */
			if (auto_change_channel(mon)) {
				net_send_channel_config();
				update_spectrum_durations();
				//update_display(NULL, NULL);
			}
		}
	}
	/* will never */
	return 0;
}

/*前期配置读取*/
int conf_read(int s)
{
	FILE *fconf;
	char tmp[4][16];
	unsigned char i = 0;
	memset(&cmds, 0x00, sizeof(cmds));
	if(MASKSCAN == s)						/*读取扫描的配置*/
	{
		if(NULL==(fconf=fopen(confscan, "r")))
		{	
			perror("confscan open fail:");
			return 0;
		}
		while((NULL != fgets(tmp[i++], 16, fconf)))
		{
			tmp[i-1][strlen(tmp[i-1])-1]='\0';
		}
		if(strncmp(tmp[0], "none", 4) == 0)
		{	
			PRINTF("disabled probe!\n");
			return 0;		//表示不启用探针
		}
		strcpy(cmds.scan_t.cifnamex, tmp[0]);
		strcpy(cmds.scan_t.csip, tmp[1]);
		strcpy(cmds.scan_t.ccmd, tmp[2]);
		strcpy(cmds.scan_t.cpara, tmp[3]);			//信道值
		PRINTF("in conf_read ifname %s \n",cmds.scan_t.cifnamex);
		PRINTF("in conf_read ip %s \n",cmds.scan_t.csip);
		PRINTF("in conf_read cmd %s \n",cmds.scan_t.ccmd);
		PRINTF("in conf_read para %s \n",cmds.scan_t.cpara);
	}
	else if(MASKCAP == s)						/*读取配置AP的配置*/
	{
	}
	else if(MASKCNET == s)						/*读取配置net的配置文件*/
	{
		
	}
	return 1;
}


/***************************************************************
*任务：执行配置结构体数据配置
*参数：
*k,是否需要关闭发送子进程
*s,三种配置的选择
***************************************************************/
int do_conf(int k, int s)
{
	char monitor_tmp[48];	
	char setap_tmp[sizeof(struct cap)+64];
	char setnet_tmp[sizeof(struct cnet)+64];
	/*是否需要关闭发送子进程*/
	if(1==k||2==k)							/*2表示关闭扫描*/
	{
		isPidScan ? kill(scanpid, SIGINT):printf("no scan process to kill \n");
		isPidUp ? kill(uppid, SIGINT):printf("no up process to kill \n");
		isPidScan = 0;
		isPidUp = 0;
	}
	if(MASKSCAN == s)						/*选择配置探针*/
	{
		strcpy(monitor_tmp, monitor);
		if(0==strncmp(cmds.scan_t.ccmd, "01", 2))		/*单信道扫描＋单信道上网*/
		{
			strcat(monitor_tmp, "1 ");
			strcat(monitor_tmp, cmds.scan_t.cifnamex);	/*接口选择*/
			strcat(monitor_tmp, " ");
			strcat(monitor_tmp, cmds.scan_t.cpara);		/*信道选择*/
			system(monitor_tmp);
			conf.do_change_channel = 0;			/*不切换信道*/
			PRINTF("test in do_conf \"01\"\n");
		}
		else if(0==strncmp(cmds.scan_t.ccmd, "02", 2))		/*全信道扫描*/
		{
			strcat(monitor_tmp, "0 ");
			strcat(monitor_tmp, cmds.scan_t.cifnamex);
			strcat(monitor_tmp, " ");
			strcat(monitor_tmp, cmds.scan_t.cpara);
			system(monitor_tmp);
			conf.do_change_channel = 1;			/*按时间切换信道*/
			PRINTF("test in do_conf \"02\"\n");
		}
		else
		{
			/*others*/
		}
	}
	else if(MASKCAP == s)					/*选择配置AP*/
	{
		/*配置AP*/
		strcpy(setap_tmp, setap);
		strcat(setap_tmp, (char*)&cmds.cap_t.conff);
		strcat(setap_tmp, " ");
		strcat(setap_tmp, cmds.cap_t.cifnamex);
		strcat(setap_tmp, " ");
		strcat(setap_tmp, cmds.cap_t.cmac);
		strcat(setap_tmp, " ");
		strcat(setap_tmp, cmds.cap_t.ccha);
		strcat(setap_tmp, " ");
		strcat(setap_tmp, cmds.cap_t.chtm);
		strcat(setap_tmp, " ");
		strcat(setap_tmp, cmds.cap_t.cpow);
		strcat(setap_tmp, " ");
		strcat(setap_tmp, cmds.cap_t.chwm);
		strcat(setap_tmp, " ");
		strcat(setap_tmp, cmds.cap_t.cssid);
		strcat(setap_tmp, " ");
		strcat(setap_tmp, cmds.cap_t.cenc);
		strcat(setap_tmp, " ");
		strcat(setap_tmp, cmds.cap_t.ckey);
		/*执行配置无线*/
		system(setap_tmp);
		/*配置完毕AP后需要重新建立扫描*/
		if(conf_read(MASKSCAN))				/*参数表示读取扫描配置文件*/
		{
			PRINTF("restart scan after config ap wireless\n");
			do_conf(1, MASKSCAN);			/*参数1表示需要关闭进程*/
		}
		
	}
	else if(MASKCNET == s)					/*选择配置本地网口参数*/
	{
		/*配置本地网口参数*/		
		strcpy(setnet_tmp, setnet);
		strcat(setnet_tmp, cmds.cnet_t.capip);
		strcat(setap_tmp, " ");
		strcat(setnet_tmp, cmds.cnet_t.capmac);
		strcat(setap_tmp, " ");
		strcat(setnet_tmp, cmds.cnet_t.capgate);
		strcat(setap_tmp, " ");
		strcat(setnet_tmp, cmds.cnet_t.capdns);
		strcat(setap_tmp, " ");
		strcat(setnet_tmp, cmds.cnet_t.capmask);
		/*执行配置有线接口*/
		system(setnet_tmp);
		/*配置完毕net后需要重新建立扫描*/
		if(conf_read(MASKSCAN))				/*参数表示读取扫描配置文件*/
		{
			PRINTF("restart scan after config ap net\n");
			do_conf(1, MASKSCAN);			/*参数1表示需要关闭进程*/
		}
	}
	if(1==k)						/*恢复子进程*/
	{
		/*扫描子进程*/
		if((isPidScan==0) && 0==(scanpid = fork()))	//执行扫描
		{
			isPidScan = 1;				//扫描进程存在标志
			PRINTF("open process for scan \n");
			do_scan();
		}
		/*上传子进程*/
		if((isPidUp==0) && 0==(uppid = fork()))		//执行扫描
		{	
			isPidUp = 1;
			PRINTF("open process for up \n");
			stations_collect();
		}
	}
	return 1;
}


/*控制主进程*/
int main()
{
        int err;            		            	/* 返回值*/

	/*系统信号抓取*/
	signal(SIGINT, sigint_handler);
	signal(SIGPIPE, sigpipe_handler);
	atexit(exit_handler);
	/*数据互斥访问*/
        scan_key = ftok(".", 's');			//扫描控制主程序部分
        main_key = ftok(".", 'm');			//主程序反馈扫描
        scan_semid = init_sem(scan_key, 0); 
        main_semid = init_sem(main_key, 0);

	system(getReayFiles);

	/*上电配置:扫描(扫描与无线开关，上传服务器IP，扫描模式)*/
	if(1 == conf_read(MASKSCAN))
	{
		do_conf(0, MASKSCAN);							//执行配置扫描，不需要关闭进程
		PRINTF("test after do_conf for MASKSCAN\n");
		/*上传子进程*/
		scanpid = fork();
		if(scanpid == 0)							//执行扫描
		{
			isPidScan = 1;							//扫描进程存在标志
			PRINTF("open process for scan \n");
			do_scan();
		}
		uppid = fork();								//子进程上传扫描结果	
		if(uppid == 0)
		{
			isPidUp = 1;							//上传进程存在
			PRINTF("open process for up \n");
			stations_collect();
		}
	}

        /*建立一个流式套接字*/
        cmd_ss = socket(AF_INET, SOCK_STREAM, 0);
        if(cmd_ss < 0){/* 出错 */
                printf("socket error\n");
                return -1;
        }
        //设置套接字选项避免地址使用错误
        int on = 1;
        if((setsockopt(cmd_ss,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)))<0)
        {
                perror("setsockopt err\n");
        }
        /*设置服务器地址*/
        bzero(&cmd_server_addr, sizeof(cmd_server_addr));               /*清０*/
        cmd_server_addr.sin_family = AF_INET;                       /* 协议族*/
        cmd_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);        /*本地地址*/
        cmd_server_addr.sin_port = htons(CMD_PORT);                 /*服务器端口*/

        /*绑定地址结构到套接字描述符*/
        err = bind(cmd_ss, (struct sockaddr*)&cmd_server_addr, sizeof(cmd_server_addr));
        if(err < 0){
                printf("bind error\n");
                return -1;
        }


        /*设置侦听*/
        err = listen(cmd_ss, 1);				/*1个时间只允许一个配置者*/
        if(err < 0){/* 出错*/
                printf("listen error\n");
                return -1;
        }

        /*接收配置主进程*/
        for( ; ; )
        {
		int addrlen = sizeof(struct sockaddr);
		/*接收客户端连接*/
		cmd_sc = accept(cmd_ss, (struct sockaddr*)&cmd_client_addr, (socklen_t *)&addrlen);
		if(cmd_sc < 0){
			continue;       			/*结束本次循环*/
		}

		process_conn_server(cmd_sc);			/*处理连接,在其中先关闭扫描子进程，处理配置完毕再重新打开子进程*/
		close(cmd_sc);              			/*关闭本次客户端的连接*/
	}
	close(cmd_sc);              				/*关闭客户端的连接*/
	close(cmd_ss);
	/*主进程异常至此，退出子进程*/
	kill(scanpid, SIGINT);
	isPidScan = 0;						/*扫描进程已经关闭*/
	kill(uppid, SIGINT);
	isPidUp = 0;						/*上传进程已经关闭*/
}

