#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

int main(int argc,char *argv[])
{
	int fd;
        fd = open("/dev/mygpio",O_RDWR,S_IRUSR|S_IWUSR);
	if(fd==-1)
	{
		printf("no this module!\n");
		return -1;
	}
	if(argc<2){printf("argc error!\n");exit(0);}
	/*while(1)
	{
        	if(fd != -1)
        	{
			ioctl(fd,1,NULL);
			sleep(1);
			ioctl(fd,0,NULL);
			sleep(1);
		}
	}*/
	switch(atoi(argv[1]))
	{
		case 0:
		ioctl(fd,0,NULL);
		break;	
		case 1:
		ioctl(fd,1,NULL);
		break;
	}
	close(fd);
	return 0;
}
