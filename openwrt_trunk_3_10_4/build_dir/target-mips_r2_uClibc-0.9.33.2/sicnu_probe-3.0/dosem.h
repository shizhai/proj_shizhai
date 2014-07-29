#ifndef __DOSEM_H
#define __DOSEM_H

#include <sys/sem.h>
#include <sys/ipc.h>

union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
};

#define deBug 1
#if deBug
#define PRINTF(...)	printf(__VA_ARGS__)
/*#define PRINTF(...)	\
	do{		\
		FILE *x;				\
		x = fopen("/tmp/testPrint","a+");	\
		fprintf(x,(char *)__VA_ARGS__);		\
		fclose(x);				\
	}while(0)*/
#else
#define PRINTF(...)	do{}while(0)
#endif

/*进程通信*/
#define m2s		'm'
#define s2m		's'

int init_sem(key_t key,int init_value);
int del_sem(int sem_id);
int sem_p(int sem_id);
int sem_v(int sem_id);

#endif
