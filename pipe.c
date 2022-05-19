#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include<fcntl.h>

#define PIPE_TEST_SIZE 2048

int count = 0;
unsigned int value = 1;

//互斥访问管道信号量(filedes)
char *pipe_name = "pipe_n";
sem_t *pipe_mutex;

//互斥访问管道信号量(size)
char *size_name = "size_n";
sem_t *size_mutex;

int main(void)
{
        int filedes[2];
        int size[2];

        if (pipe(filedes) == -1)
                return -1;
        if (pipe(size) == -1)
                return -1;

        //创建有名信号量
        pipe_mutex=sem_open(pipe_name,O_CREAT,0644,value);
	    size_mutex=sem_open(size_name,O_CREAT,0644,value);

        printf("this is parent, the test begins.\n");

        pid_t pid1;

        if ((pid1 = fork()) == 0)
        {
                if (fork() == 0)
                {
                        if (fork() == 0)
                        {
                                prctl(PR_SET_PDEATHSIG, SIGKILL);
                                usleep(10);
                                close(filedes[0]);
                                close(size[0]);

                                while (1)
                                {
                                        sem_wait(pipe_mutex);
                                        if (write(filedes[1], "child3 input", PIPE_TEST_SIZE) > 0)
                                        {
                                                sem_post(pipe_mutex);
                                                sem_wait(size_mutex);
                                                write(size[1], "3", 1);
                                                sem_post(size_mutex);

                                                count += 1;
                                                printf("child3 times:%d\n", count);
                                                usleep(100);
                                        }
                                        else
                                        {
                                                sem_post(pipe_mutex);
                                                printf("write error!\n");
                                        }
                                }
                        }

                        prctl(PR_SET_PDEATHSIG, SIGKILL);
                        usleep(10);
                        close(filedes[0]);
                        close(size[0]);

                        while (1)
                        {
                                sem_wait(pipe_mutex);
                                if (write(filedes[1], "child2 input", PIPE_TEST_SIZE) > 0)
                                {
                                        sem_post(pipe_mutex);
                                        sem_wait(size_mutex);
                                        write(size[1], "2", 1);
                                         usleep(12000);//test semaphore
                                        sem_post(size_mutex);

                                        count += 1;
                                        printf("child2 times:%d\n", count);
                                        usleep(100);
                                }
                                else
                                {
                                        sem_post(pipe_mutex);
                                        printf("write error!\n");
                                }
                        }
                }

                usleep(10);
                close(filedes[0]);
                close(size[0]);

                while (1)
                {
                        sem_wait(pipe_mutex);

                        if (write(filedes[1], "child1 input", PIPE_TEST_SIZE) > 0)
                        {
                                sem_post(pipe_mutex);
                                sem_wait(size_mutex);
                                write(size[1], "1", 1);
                                usleep(5000);//test semaphore
                                sem_post(size_mutex);

                                count += 1;
                                printf("child1 times:%d\n", count);
                                usleep(100);
                        }
                        else
                        {
                                sem_post(pipe_mutex);
                                printf("write error!\n");
                        }
                }
        }

        printf("this is parent, waiting...\n");
        usleep(1000000);

        printf("this is parent, the test is end.\n");

        char buf[64];
        sem_wait(size_mutex);
        read(size[0], buf, 64);
        count = strlen(buf);
        count -= 1 ;//出去结尾所占字符
        sem_post(size_mutex);
        printf("buf:%s\n", buf);
        printf("the default size of pipe is:%d\n", count * PIPE_TEST_SIZE);

        kill(pid1, SIGKILL);
        usleep(100000);

        char read_buf[2 * PIPE_TEST_SIZE];
        printf("the following is what is read by parent:\n");
        close(filedes[1]);
        for (int i = 0; i < count; i++)
        {
                read(filedes[0], read_buf, 2 * PIPE_TEST_SIZE);
                printf("%s ", read_buf);
                if (i % 7 == 6)
                        printf("\n");
        }

        printf("\nend.\n");

        close(filedes[1]);
        close(size[1]);

        //关闭信号量
        if (sem_close(pipe_mutex) == -1)
                return -1;
        if (sem_close(size_mutex) == -1)
                return -1;

        //删除信号量
        if (sem_unlink(pipe_name) == -1)
                return -1;
        if (sem_unlink(size_name) == -1)
                return -1;

        return 0;
}
