#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include<sys/types.h>

#define MAX_BUF_LEN 1028

int IPC_PIPE(void)
{
    pid_t pid;
    int pipefd[2] = {0,};
    char buf_tx[MAX_BUF_LEN];
    char buf_rx[MAX_BUF_LEN];

    memset((void *)buf_tx, 0, sizeof(buf_tx));
    snprintf(buf_tx, sizeof(buf_tx), "msg: %s", "Hello world");

    if(pipe(pipefd) == -1)
    {
        perror("pipe()");
        return -1;
    }

    pid = fork();

    if(pid == 0)
    {
        close(pipefd[0]);
        printf("[Send Data] - [PARENT] pid: %u, %s\n",getpid(), buf_tx);
        if(write(pipefd[1], (void *)buf_tx, sizeof(buf_tx)) == -1)
        {
            perror("write()");
            return -1;
        }
        close(pipefd[1]);
    }
    else if(pid > 0)
    {
        close(pipefd[1]);
        memset((void *)buf_rx, 0, sizeof(buf_rx));
        if(read(pipefd[0], (void *)buf_rx, sizeof(buf_rx)) == -1)
        {
            perror("read()");
            return -1;
        }
        printf("[Recv Data] - [CHILD] pid: %u, %s\n",getpid(),buf_rx);
        close(pipefd[0]);
    }
    else
    {
        perror("fork()");
        return -1;
    }

    return 0;
}