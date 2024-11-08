#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <sys/stat.h>

int main() {
    mqd_t mq;
    struct mq_attr attr;
    char *msg = "Hello, POSIX Message Queue!";

    // 메시지 큐 속성 설정
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 128;
    attr.mq_curmsgs = 0;

    // 메시지 큐 생성
    mq = mq_open("/test_mq", O_CREAT | O_WRONLY, 0644, &attr); // "/test_mq" 이름으로 mq 생성. 이 mq는 프로세스간 접근할 수 있음.
    if (mq == -1) {
        perror("Message queue creation failed");
        exit(1);
    }

    // 메시지 전송
    if (mq_send(mq, msg, strlen(msg) + 1, 0) == -1) {
        perror("Message send failed");
        exit(1);
    }

    printf("Message sent\n");

    // 메시지 큐 닫기
    mq_close(mq);

    return 0;
}