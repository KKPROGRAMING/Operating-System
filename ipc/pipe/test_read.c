// pipe test
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <fcntl.h>

#define SIZE 32

sem_t *mutex;
char *mutex_name = "mutex";
int mutex_value = 1;

int main()
{
    //创建管道
    int filedes[2];
    if (pipe(filedes) == -1)
        return -1;

    //初始化有名信号量
    mutex = sem_open(mutex_name, O_CREAT, 0644, mutex_value);

    //父进程读，子进程写
    //关闭父进程写管道，关闭子进程读管道
    if (fork() == 0)
    {
        close(filedes[0]);

        //有睡眠写进程
        sem_wait(mutex);
        write(filedes[1], "helloworld", 10);
        printf("(Child)wrote 10. >>>helloworld\n");
        printf("(Child)sleeping......\n");
        sem_post(mutex);
        usleep(1000000);


        //无睡眠写进程
        printf("(Child)awake!\n");
        //执行下面的写操作时，p=0，读进程等待写进程写数据，然后再读取数据
        write(filedes[1], "bonjourworld", 12);
        printf("(Child)wrote 12. >>>bonjourworld\n");
        usleep(500);

        //最后写入一次数据，关闭写进程
        write(filedes[1], "goodbyeworld", 12);
        printf("(Child)wrote 12. >>>goodbyeworld\n");
        close(filedes[1]);
        printf("(Child)killed!\n");
        return 0;
    }
    else
    {
        int ret;
        close(filedes[1]);
        char buff[SIZE];

        //先等待写进程写入一些东西
        usleep(500);

        //有睡眠写进程
        sem_wait(mutex);
        // p!=0,p>n
        memset(buff,0,SIZE);
        ret = read(filedes[0], buff, 2);
        printf("(Parent)read 2, return %d, the content is:%s\n", ret, buff);
        // p!=0,0<p<n
        memset(buff,0,SIZE);
        ret = read(filedes[0], buff, 12);
        printf("(Parent)read 12, return %d, the content is:%s\n", ret, buff);
        // p=0,waiting(有睡眠写) -> p=0,waiting(无睡眠写) -> p!=0,p>n(无睡眠写)
        //读进程等待写进程写数据，然后再读取数据
        memset(buff,0,SIZE);
        ret = read(filedes[0], buff, 2);
        printf("(Parent)read 2, return %d, the content is:%s\n", ret, buff);
        sem_post(mutex);

        //无睡眠写进程
        // p!=0,0<p<n，理想情况返回0，p变为0，读进程不等待
        ret = read(filedes[0], buff, 24);
        printf("(Parent)read 24, return %d, the content is:%s\n", ret, buff);

        //等待写进程最后一次写入数据，没有写进程
        usleep(500);
        // p!=0,p>n
        memset(buff,0,SIZE);
        ret = read(filedes[0], buff, 2);
        printf("(Parent)read 2, return %d, the content is:%s\n", ret, buff);
        // 0<p<n
        memset(buff,0,SIZE);
        ret = read(filedes[0], buff, 16);
        printf("(Parent)read 16, return %d, the content is:%s\n", ret, buff);
        // p=0
        memset(buff,0,SIZE);
        ret = read(filedes[0], buff, 2);
        printf("(Parent)read 2, return %d, the content is:%s\n", ret, buff);
    }

    //关闭信号量
    if (sem_close(mutex) == -1)
        return -1;

    //删除信号量
    if (sem_unlink(mutex_name) == -1)
        return -1;
    return 0;
}
