#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "dosem.h"

/*将信号sem_id设置为init_value*/
int init_sem(key_t key,int init_value)
{
        union semun sem_union;
        int semid;
        sem_union.val=init_value;

        semid = semget(key, 1, IPC_CREAT | 0666);
        if(-1 == semid)
        {
                printf("create semaphore error in server\n");
                return -1;
        }
        if (semctl(semid,0,SETVAL,sem_union)==-1)
        {
                perror("Sem init in server");
                return -2;
        }
        return semid;
}
/*删除sem_id信号量*/
int del_sem(int sem_id)
{
        union semun sem_union;
        if (semctl(sem_id,0,IPC_RMID,sem_union)==-1)
        {
                perror("Sem delete server");
                exit(1);
        }
        return 0;
}
/*对sem_id执行p操作*/
int sem_p(int sem_id)
{
        struct sembuf sem_buf;
        sem_buf.sem_num=0;              //信号量编号
        sem_buf.sem_op=1;              //Ｐ操作
        sem_buf.sem_flg=SEM_UNDO;       //系统退出时释放信号量，系统自动释放，并且默认为阻塞
        if (semop(sem_id,&sem_buf,1)==-1) {
                perror("Sem P operation in server");
                exit(1);
        }
        return 0;
}

/*对sem_id执行v操作*/
int sem_v(int sem_id)
{
        struct sembuf sem_buf;
        sem_buf.sem_num=0;
        sem_buf.sem_op=-1;               //V操作
        sem_buf.sem_flg=SEM_UNDO;
        if (semop(sem_id,&sem_buf,1)==-1)
        {
                perror("Sem V operation in server");
                exit(1);
        }
        return 0;
}
