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
        write(filedes[1],"helloworld",SIZE);

    //父进程写，子进程读
    if(fork()==0){
        char buff[SIZE];
        close(filedes[1]);

        printf("(Child 1)sleeping......\n");
        usleep(50000);

        printf("(Child 2)awake!\n");
        read(filedes[0],buff,SIZE);
        printf("(Child)read %d Bytes.\n",SIZE);

        //分两次读出，每次读出4096B
        //no sleep!否则写进程会认为没有读进程而退出
        read(filedes[0],buff,SIZE);
        printf("(Child 4)read %d Bytes.\n",SIZE);
        read(filedes[0],buff,SIZE);
        printf("(Child 5)read %d Bytes.\n",SIZE);

        //杀死读进程
        close(filedes[0]);
        printf("(Child 6)killed!\n");
        return 0 ;

    }
    else{
        int ret;
        close(filedes[0]);

        usleep(500);
        //u>n,u=2*4096,n=4096
        ret = write(filedes[1],"helloworld",SIZE);
        printf("(Parent 1)wrote %d, return %d.\n",SIZE,ret);
        //u=n,u=4096,n=4096
        ret = write(filedes[1],"helloworld",SIZE);
        printf("(Parent 2)wrote %d, return %d.\n",SIZE,ret);

        //u<n<4096,u=0,n=4096
        //等待，直到有n-u=4096字节被读出，写入n字节并返回n
        ret = write(filedes[1],"helloworld",SIZE);
        printf("(Parent 3)wrote %d, return %d.\n",SIZE,ret);
        //usleep(200);

        //u<n,u=0,n>4096(n=4096+4)
        ret = write(filedes[1],"helloworld",SIZE+4);
        printf("(Parent 4)wrote %d, return %d.\n",SIZE+4,ret);
        usleep(200);

        //没有读进程，u>0，尝试写入n=1
        //预期结果是写入失败，进程返回-EPIPE
        ret = write(filedes[1],"t",1);
        printf("(Parent 5)wrote %d, return %d.\n",1,ret);

        close(filedes[1]);
        return 0;      
    }
}
