#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <sys/stat.h>

int main() {
    mqd_t mq;
    struct mq_attr attr;
    char buffer[128];

    // 메시지 큐 오픈
    mq = mq_open("/test_mq", O_RDONLY);
    if (mq == -1) {
        perror("Message queue open failed");
        exit(1);
    }

    // 메시지 수신
    if (mq_receive(mq, buffer, 128, NULL) == -1) {
        perror("Message receive failed");
        exit(1);
    }

    printf("Received message: %s\n", buffer);

    // 메시지 큐 닫기
    mq_close(mq);
    // 메시지 큐 삭제
    mq_unlink("/test_mq");

    return 0;
}