#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define SIZE 4096

int filedes[2];

int main(){
    //创建管道
    if(pipe(filedes)==-1)
        return -1 ;

    //写内容进入管道，直到留下2*4096空间
    for(int i=1 ; i<=14 ; i++)
        write(fildes[1],"helloworld",SIZE);

    //父进程写，子进程读
    if(fork()==0){
        char buff[SIZE];
        close(filedes[1]);

        printf("(Child)sleeping......\n");
        usleep(50000);

        printf("(Child)awake!\n");
        read(filedes[0],buff,SIZE);
        printf("(Child)read %d Bytes.\n");

        printf("(Child)sleeping......\n");
        usleep(50000);

        printf("(Child)awake!\n");
        //分两次读出，每次读出4096B
        read(filedes[0],buff,SIZE);
        printf("(Child)read %d Bytes.\n");
        read(filedes[0],buff,SIZE);
        printf("(Child)read %d Bytes.\n");

        //杀死读进程
        close(fildes[0]);
        printf("(Child)killed!");
        return 0 ;

    }
    else{
        int ret;
        close(filedes[0]);

        usleep(500);
        //u>n,u=2*4096,n=4096
        ret = write(filedes[1],"helloworld",SIZE);
        printf("(Parent)wrote %d, return %d.\n",SIZE,ret);
        //u=n,u=4096,n=4096
        ret = write(filedes[1],"helloworld",SIZE);
        printf("(Parent)wrote %d, return %d.\n",SIZE,ret);

        //u<n<4096,u=0,n=4096
        //等待，直到有n-u=4096字节被读出，写入n字节并返回n
        ret = write(filedes[1],"helloworld",SIZE);
        printf("(Parent)wrote %d, return %d.\n",SIZE,ret);

        //u<n,n>4096(n=4096+4)
        ret = write(filedes[1],"helloworld",SIZE+4);
        printf("(Parent)wrote %d, return %d.\n",SIZE+4,ret);
        usleep(200);

        //没有读进程，u>0，尝试写入n=1
        //预期结果是写入失败，返回-EPIPE
        ret = write(filedes[1],"t",1);
        printf("(Parent)wrote %d, return %d.\n",1,ret);

        return 0;      
    }
}