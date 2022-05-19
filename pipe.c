// change4
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/prctl.h>

#define PIPE_TEST_SIZE 2048

int count = 0;

int main(void)
{
        int filedes[2];
        int size[2];

        if (pipe(filedes) == -1)
                return -1;
        if (pipe(size) == -1)
                return -1;

        //互斥访问管道信号量(filedes)
        sem_t pipe_mutex;
        int pshared = 4;
        unsigned int value = 1;
        if (sem_init(&pipe_mutex, pshared, value) == -1)
                return -1;

        //互斥访问管道信号量(size)
        sem_t size_mutex;
        if (sem_init(&size_mutex, pshared, value) == -1)
                return -1;

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
                                        sem_wait(&pipe_mutex);

                                        if (write(filedes[1], "child3 input", PIPE_TEST_SIZE) > 0)
                                        {
                                                sem_post(&pipe_mutex);
                                                sem_wait(&size_mutex);
                                                write(size[1], "3", 1);
                                                sem_post(&size_mutex);

                                                count += 1;
                                                printf("child3 times:%d\n", count);
                                                usleep(100);
                                        }
                                        else
                                        {
                                                sem_post(&pipe_mutex);
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
                                sem_wait(&pipe_mutex);

                                if (write(filedes[1], "child2 input", PIPE_TEST_SIZE) > 0)
                                {
                                        sem_post(&pipe_mutex);
                                        sem_wait(&size_mutex);
                                        write(size[1], "2", 1);
                                        sem_post(&size_mutex);

                                        count += 1;
                                        printf("child2 times:%d\n", count);
                                        usleep(100);
                                }
                                else
                                {
                                        sem_post(&pipe_mutex);
                                        printf("write error!\n");
                                }
                        }
                }

                usleep(10);
                close(filedes[0]);
                close(size[0]);

                while (1)
                {
                        sem_wait(&pipe_mutex);

                        if (write(filedes[1], "child1 input", PIPE_TEST_SIZE) > 0)
                        {
                                sem_post(&pipe_mutex);
                                sem_wait(&size_mutex);
                                write(size[1], "1", 1);
                                sem_post(&size_mutex);

                                count += 1;
                                printf("child1 times:%d\n", count);
                                usleep(100);
                        }
                        else
                        {
                                sem_post(&pipe_mutex);
                                printf("write error!\n");
                        }
                }
        }

        printf("this is parent, waiting...\n");
        usleep(100000);

        printf("this is parent, the test is end.\n");

        char buf[64];
        sem_wait(&size_mutex);
        read(size[0], buf, 64);
        count = strlen(buf);
        sem_post(&size_mutex);
        printf("buf:%s\n", buf);
        printf("the default size of pipe is:%d\n", count * PIPE_TEST_SIZE);

        kill(pid1, SIGKILL);
        usleep(100000);

        char read_buf[2 * PIPE_TEST_SIZE];
        sem_wait(&pipe_mutex);
        printf("the following is what is read by parent:\n");
        close(filedes[1]);
        for (int i = 0; i < count; i++)
        {
                read(filedes[0], read_buf, 2 * PIPE_TEST_SIZE);
                printf("%s ", read_buf);
                if (i % 7 == 6)
                        printf("\n");
        }
        sem_post(&pipe_mutex);

        printf("\nend.\n");

        close(filedes[1]);
        close(size[1]);

        if (sem_destroy(&pipe_mutex) == -1)
                return -1;
        if (sem_destroy(&size_mutex) == -1)
                return -1;

        return 0;
}
