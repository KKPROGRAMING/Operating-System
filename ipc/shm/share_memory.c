#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>

#define SIZE 256

//互斥访问共享内存
sem_t *mutex;
char *mutex_name = "mutex";

//等待sender消息
sem_t *sender_m;
char *sender_m_name = "sender_m";

//等待receiver消息
sem_t *receiver_m;
char *receiver_m_name = "receiver_m";

//等待任务全部结束
sem_t *finish;
char *finish_name = "finish";

int mutex_value = 1;
int message_value = 0;
int finish_value = 0;

char *filename = "shm.c";
int shmid;
key_t key;
char *buff;

int sender()
{
    if ((key = ftok(filename, 111)) == -1)
        return -1;

    if ((shmid = shmget(key, SIZE, IPC_CREAT)) == -1)
        return -1;

    sem_wait(mutex);
    buff = shmat(shmid, NULL, 0);
    char input[SIZE];
    printf("(Sender)please input something:");
    //scanf("%s", &input);
    gets(input);
    bzero(buff, SIZE);
    strcpy(buff, input);
    sem_post(mutex);
    sem_post(sender_m);

    sem_wait(receiver_m);
    sem_wait(mutex);
    char output[SIZE];
    strcpy(output, buff);
    printf("(Sender)received:%s\n", output);
    sem_post(mutex);

    if (shmdt(buff) == -1)
        return -1;

    if (shmctl(shmid, IPC_RMID, NULL) == -1)
        return -1;

    sem_post(finish);
    return 0 ;
}

int receiver()
{
    if ((key = ftok(filename, 111)) == -1)
        return -1;

    if ((shmid = shmget(key, SIZE, IPC_CREAT)) == -1)
        return -1;

    sem_wait(sender_m);
    sem_wait(mutex);
    buff = shmat(shmid, NULL, 0);
    char output[SIZE];
    strcpy(output, buff);
    printf("(Receiver)received:%s\n", output);
    printf("(Receiver)send:over\n");
    bzero(buff, SIZE);
    strcpy(buff, "over");
    sem_post(mutex);
    sem_post(receiver_m);

    if (shmdt(buff) == -1)
        return -1;
    return 0;
}

int main()
{
    mutex = sem_open(mutex_name, O_CREAT, 0644, mutex_value);
    sender_m = sem_open(sender_m_name, O_CREAT, 0644, message_value);
    receiver_m = sem_open(receiver_m_name, O_CREAT, 0644, message_value);
    finish = sem_open(finish_name, O_CREAT, 0644, finish_value);

    int pid_sender, pid_receiver;
    pid_sender = pid_receiver = -1;

    if ((pid_sender = fork()) == 0)
        sender();
    else
    {
        usleep(500);
        if ((pid_receiver = fork()) == 0)
            receiver();
    }

    if ((pid_sender != 0) && (pid_receiver != 0)){
        sem_wait(finish);
        printf("(Main)the test is end!\n");
    }
    return 0;
}
