#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define SHM_NAME "/my_shared_memory"
#define SEM_NAME "/my_semaphore"

typedef struct 
{
    __uint32_t pid;
    __uint32_t recv_flag;
    __uint32_t recv_player;
    char data[1024];
}shared_data;

static char temp[1024] = {0,};

static void mask_receive_flag(__uint32_t *flag, __uint32_t TaskNum);
static void clear_receive_flag(__uint32_t *flag);
volatile int key_pressed = 0;

static sem_t *g_sem;
static shared_data *g_shared_mem;

void* input_thread(void* arg) {
    char c;
    __uint32_t i = 0;
    __uint32_t count;
    while (1) 
    {
        while(1)
        {
            g_shared_mem->data[i] = getchar();

            if(temp[i] == '\n')
            {
                g_shared_mem->pid = getpid();
                break;
            }

            i ++;
        };
        
        printf("문자: %s\n", temp);

        for(count = 0; count < 2; count ++)
        {
            printf("sem address: %p",g_sem);
            sem_post(g_sem);
        }

        usleep(100000);  // CPU 사용을 줄이기 위해 잠시 대기

        i = 0;
    }
    return NULL;
}

void writer_process() 
{
    // 공유 메모리 및 세마포어 생성
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(int));
    shared_data* shared_mem = mmap(0, sizeof(shared_data), PROT_WRITE, MAP_SHARED, shm_fd, 0);
    __uint32_t count;

    memset(shared_mem, 0, sizeof(shared_mem));
    
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 0);

    do
    {
        scanf("%s", shared_mem->data);
        shared_mem->pid = getpid();

        /* 모든 task에게 알림. */
        for(count = 0; count < shared_mem->recv_player; count ++)
        {
            sem_post(sem);
        }

    } while (1);

    // 정리
    munmap(shared_mem, sizeof(int));
    close(shm_fd);
    sem_close(sem);
}

void reader_process() {
    // 공유 메모리 및 세마포어 열기
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    shared_data *g_shared_mem = mmap(0, sizeof(shared_data), O_RDWR, MAP_SHARED, shm_fd, 0);
    
    g_shared_mem->recv_player ++;
    printf("recv palyer count: %d\n",g_shared_mem->recv_player);

    g_sem = sem_open(SEM_NAME, 0);
    printf("sem address: %p",g_sem);

    do
    {
        /* 세마포어 알림 기다리기. */
        sem_wait(g_sem);

        /* 알림 받은 후 읽기. */
        printf("%d: %s\n", g_shared_mem->pid, g_shared_mem->data);
    } while (strcmp((char *)g_shared_mem, "END") != 0);

    // 정리
    munmap(g_shared_mem, sizeof(int));
    close(shm_fd);
    sem_close(g_sem);
}

int main(int argc, char* argv[]) 
{
    pthread_t thread;

    if (argc < 2) {
        fprintf(stderr, "사용법: %s [writer|reader]\n", argv[0]);
        exit(1);
    }

    if (strcmp(argv[1], "writer") == 0) {
        writer_process();
    } else if (strcmp(argv[1], "reader") == 0) {
        pthread_create(&thread, NULL, input_thread, NULL);
        reader_process();
    }

    return 0;
}

static void mask_receive_flag(__uint32_t *flag, __uint32_t TaskNum)
{
    *flag |= (1 <<  TaskNum);
}

static void clear_receive_flag(__uint32_t *flag)
{
    *flag = 0;
}