#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>

// msgtype
#define send1_to_recv 1
#define send2_to_recv 2
#define recv_to_send1 3
#define recv_to_send2 4

#define SIZE 128

//自定义消息缓冲区结构
struct msgbuf
{
    long mtype;
    char mtext[SIZE];
};

// sender1结束信号
sem_t final1;
// sender2结束信号
sem_t final2;

int msgid;
pthread_t sender1_pid, sender2_pid, receiver_pid;

//判断发送方的信息是否发送完毕
int over1 = 0;
int over2 = 0;

void *sender1(void *arg)
{
    char buff[SIZE];
    struct msgbuf s_msg;
    s_msg.mtype = send1_to_recv;
    
    usleep(50);

    while (1)
    {
        memset(buff, 0, SIZE);
        gets(buff);

        //比较内容是否等于“exit”，是则发送“end1”并break
        if (strcmp(buff, "exit") == 0)
            strcpy(s_msg.mtext, "end1");
        else
            strcpy(s_msg.mtext, buff);

        if (msgsnd(msgid, &s_msg, SIZE, 0) != 0)
            exit(1);

        if (strcmp(buff, "exit") == 0)
        {
            //等待receiver传回结束消息
            sem_wait(&final1);

            msgrcv(msgid, &s_msg, SIZE, recv_to_send1, 0);
            printf("(Sender1) received:%s\n", s_msg.mtext);
            break;
        }
    }

    pthread_exit(NULL);
}

void *sender2(void *arg)
{
    char buff[SIZE];
    struct msgbuf s_msg;
    s_msg.mtype = send2_to_recv;

    while (1)
    {
        memset(buff, 0, SIZE);
        gets(buff);

        //比较内容是否等于“exit”，是则发送“end2”并break
        if (strcmp(buff, "exit") == 0)
            strcpy(s_msg.mtext, "end2");
        else
            strcpy(s_msg.mtext, buff);

        if (msgsnd(msgid, &s_msg, SIZE, 0) != 0)
            exit(1);

        if (strcmp(buff, "exit") == 0)
        {
            //等待receiver传回结束消息
            sem_wait(&final2);

            msgrcv(msgid, &s_msg, SIZE, recv_to_send2, 0);
            printf("(Sender2) received:%s\n", s_msg.mtext);
            break;
        }
    }

    pthread_exit(NULL);
}

void *receiver(void *arg)
{
    struct msgbuf r_msg; //接收缓冲区
    while (1)
    {
        //接收消息队列里的第一个,如果没有可以接受的消息，则立即返回-1
        if (msgrcv(msgid, &r_msg, SIZE, 0, IPC_NOWAIT) == -1)
            if (over1 && over2)
                break;
            else
                continue;

        if (r_msg.mtype == send1_to_recv)
        {
            if (strcmp(r_msg.mtext, "end1") == 0)
            {
                strcpy(r_msg.mtext, "over1");
                r_msg.mtype = recv_to_send1;

                if (msgsnd(msgid, &r_msg, SIZE, IPC_NOWAIT) == -1)
                    exit(1);

                sem_post(&final1);

                over1 = 1;
            }
            else
            {
                printf("(Receiver)receive from sender1 >>>%s\n", r_msg.mtext);
            }
        }
        else if (r_msg.mtype == send2_to_recv)
        {
            if (strcmp(r_msg.mtext, "end2") == 0)
            {
                strcpy(r_msg.mtext, "over2");
                r_msg.mtype = recv_to_send2;

                if (msgsnd(msgid, &r_msg, SIZE, IPC_NOWAIT) == -1)
                    exit(1);

                sem_post(&final2);

                over2 = 1;
            }
            else
            {
                printf("(Receiver)receive from sender2 >>>%s\n", r_msg.mtext);
            }
        }

        usleep(500);
    }

    //删除消息队列并退出
    msgctl(msgid, IPC_RMID, NULL);
    pthread_exit(NULL);
}

int main(void)
{
    //初始化信号量
    if (sem_init(&final1, 0, 0) == -1)
        return -1;
    if (sem_init(&final2, 0, 0) == -1)
        return -1;

    //创建消息队列
    if ((msgid = msgget(IPC_PRIVATE, IPC_CREAT)) == -1)
        return -2;

    //创建线程
    if (pthread_create(&sender1_pid, NULL, sender1, NULL) != 0)
        return -3;
    if (pthread_create(&sender2_pid, NULL, sender2, NULL) != 0)
        return -3;
    if (pthread_create(&receiver_pid, NULL, receiver, NULL) != 0)
        return -3;

    //等待线程结束
    pthread_join(sender1_pid, NULL);
    pthread_join(sender2_pid, NULL);
    pthread_join(receiver_pid, NULL);

    //删除信号量
    if (sem_destroy(&final1) == -1)
        return -1;
    if (sem_destroy(&final2) == -1)
        return -1;

    return 0;
}
