#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <error.h>

#define SHM_NAME "/my_shared_memory"
#define SEM_NAME "/my_semaphore"

#define MAX_USER    12

typedef struct 
{
    __uint32_t pid;
    __uint32_t recv_flag;
    __uint32_t recv_player;
    sem_t *user_sem[MAX_USER];
    char sem_name[MAX_USER][255];
    char data[1024];
}shared_data;

static char temp[1024] = {0,};

static void mask_receive_flag(__uint32_t *flag, __uint32_t TaskNum);
static void clear_receive_flag(__uint32_t *flag);
volatile int key_pressed = 0;

static sem_t *g_sem;
static shared_data *g_shared_mem;

static __uint32_t flag = 1;

void *input_thread(void* shm) 
{
    u_int8_t count;
    shared_data *th_shm = (shared_data *)shm; 
    printf("task start!\n");
    while(1) 
    {
        for(count = 0; count < th_shm->recv_player; count ++)
        {
            printf("wait task%d!\n",count);
            sem_wait(th_shm->user_sem[count]);
            printf("From: user%d: %s \n",count, th_shm->data);
        }
    }
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

void recv_thread(sem_t *user_sem) 
{
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
        sem_wait(user_sem);

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
    int shm_fd;
    int error;
    if(argc > 1)
    {
        if(strcmp(argv[1], "server") == 0)
        {
            sem_t *server_sem;
            pthread_t thread;
            shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
            if(shm_fd == -1)
            {
                printf("create shared mem!\n");
                shm_fd = shm_open(SHM_NAME, (O_CREAT | O_RDWR), 0666);
                ftruncate(shm_fd, sizeof(g_shared_mem));  // 공유 메모리의 사이즈를 설정한다.
                g_shared_mem =mmap(0, sizeof(shared_data), O_RDWR, MAP_SHARED, shm_fd, 0);
                
                // strcpy(g_shared_mem->sem_name[0], "test sem name");
                error = pthread_create(&thread, NULL, input_thread, (void *)g_shared_mem);
                if(error != 0)
                {
                    switch(error)
                    {
                        case 1:
                            printf("The caller does not have the necessary permissions to set scheduling attributes.\n");
                        break;

                        case 11:
                            printf("Insufficient resources to create another thread, or a system-imposed limit was reached.\n");
                        break;

                        case 22:
                            printf("Invalid settings in the pthread_attr_t object or invalid arguments to pthread_create.\n");
                        break;
                    }
                }
                else
                {
                    printf("create thread!\n");
                }

                pthread_join(thread, NULL);

            }
            else
            {
                printf("Close previous shared mem, semaphore!\n");

                // shm_unlink(SHM_NAME);
                // if(sem_close(server_sem) == -1)
                // {
                //     perror("sem_close failed!\n");
                // }
                // if(sem_unlink(SEM_NAME) == -1)
                // {
                //     perror("sem_unlink failed!\n");
                // }

                close(shm_fd);

                printf("Open new shared mem, semaphore!\n");

                shm_fd = shm_open(SHM_NAME, (O_CREAT | O_RDWR), 0666);
                ftruncate(shm_fd, sizeof(g_shared_mem));  // 공유 메모리의 사이즈를 설정한다.
                g_shared_mem =mmap(0, sizeof(shared_data), O_RDWR, MAP_SHARED, shm_fd, 0);
            }
        }
        else if(strcmp(argv[1], "user") == 0)
        {
            char sem_name[255];

            shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
            if(shm_fd == -1)
            {
                perror("server is close! please create server\n");
                exit(1);
            }

            sem_t *user_sem;
            
            g_shared_mem =mmap(0, sizeof(shared_data), O_RDWR, MAP_SHARED, shm_fd, 0);
            sprintf(sem_name, "user sem %d",g_shared_mem->recv_player);
            
            user_sem = sem_open(sem_name, 0);

            g_shared_mem->user_sem[g_shared_mem->recv_player] = user_sem;
            strcpy(g_shared_mem->sem_name[g_shared_mem->recv_player], sem_name);

            g_shared_mem->recv_player ++;

        }
        else
        {
            printf("please select server or user!\n");
            printf("Usage ./main server or user\n");
        }
    }
    else
    {
        printf("Usage ./main server or user\n");
    }

    
        

    munmap(g_shared_mem, sizeof(shared_data));
    shm_unlink(SHM_NAME);
    close(shm_fd);

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