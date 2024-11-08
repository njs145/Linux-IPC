#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>

int main() {
    int shm_id;
    key_t key = 1234; // 공유 메모리 키
    char *shm_ptr;

    // 공유 메모리 세그먼트 생성
    shm_id = shmget(key, 1024, IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget");
        exit(1);
    }

    // 공유 메모리 세그먼트를 현재 프로세스의 주소 공간에 첨부
    shm_ptr = shmat(shm_id, NULL, 0);
    if (shm_ptr == (char *) -1) {
        perror("shmat");
        exit(1);
    }

    // 공유 메모리에 데이터 쓰기
    printf("Enter some text: ");
    fgets(shm_ptr, 1024, stdin);

    printf("Written to shared memory: %s\n", shm_ptr);

    // 프로세스가 공유 메모리 세그먼트를 더 이상 사용하지 않음을 시스템에 알림
    shmdt(shm_ptr);

    return 0;
}