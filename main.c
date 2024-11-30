/*---------------------------------------------------------------------------------------------------
********************************************* INCLUDES **********************************************
---------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/*---------------------------------------------------------------------------------------------------
********************************************** DEFINE ***********************************************
---------------------------------------------------------------------------------------------------*/

#define SHM_NAME "/my_shared_memory"
#define SEM_NAME "/my_semaphore"

/*---------------------------------------------------------------------------------------------------
************************************ GLOBAL VARIABLE DEFINITIONS ************************************
---------------------------------------------------------------------------------------------------*/

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

volatile int key_pressed = 0;

static sem_t *g_sem;
static shared_data *g_shared_mem;

static __uint32_t flag = 1;

/*----------------------------------------------------------------------------------------------------
************************************* LOCAL CONSTANT DEFINITIONS *************************************
----------------------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------------------
*********************************** IMPORTED VARIABLE DECLARATIONS ***********************************
----------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------
************************************* LOCAL FUNCTION DEFINITIONS ************************************
---------------------------------------------------------------------------------------------------*/

static void run_server(int *shm_fd);
static void run_user(int *shm_fd);

/*----------------------------------------------------------------------------------------------------
**************************************** FUNCTION DEFINITIONS ****************************************
----------------------------------------------------------------------------------------------------*/

void *input_thread(void* shm) 
{
    u_int8_t user_num = 1;
    shared_data *th_shm = (shared_data *)shm; 
    u_int32_t count = 1;
    printf("task start!\n");
    while(1) 
    {
        for(user_num = 1; user_num <= th_shm->recv_player; user_num ++)
        {
            if(th_shm->user_sem[user_num] == NULL)
            {
                th_shm->user_sem[user_num] = sem_open(th_shm->sem_name[user_num], 0);
            }
            printf("wait task%d![%d]\n",user_num,count);
            sem_wait(th_shm->user_sem[user_num]);
            printf("From: user%d: %s \n",user_num, th_shm->data);
        }
        count ++;
        // sleep(1);
    }
}

void writer_process() 
{
    // 공유 메모리 및 세마포어 생성
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(int));
    shared_data* shared_mem = mmap(0, sizeof(shared_data), PROT_WRITE, MAP_SHARED, shm_fd, 0);
    __uint32_t user_num;
    
    memset(shared_mem, 0, sizeof(shared_mem));
    
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 0);

    do
    {
        scanf("%s", shared_mem->data);
        shared_mem->pid = getpid();

        /* 모든 task에게 알림. */
        for(user_num = 0; user_num < shared_mem->recv_player; user_num ++)
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
    printf("recv palyer user_num: %d\n",g_shared_mem->recv_player);

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
            run_server(&shm_fd);
        }
        else if(strcmp(argv[1], "user") == 0)
        {
            run_user(&shm_fd);
        }
    }
    else
    {
        run_server(&shm_fd);
        // run_user(&shm_fd);
    }

    munmap(g_shared_mem, sizeof(shared_data));
    shm_unlink(SHM_NAME);
    close(shm_fd);

    return 0;
}

static void run_server(int *shm_fd)
{
    sem_t *server_sem;
    pthread_t thread;
    int error;

    *shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if(*shm_fd == -1)
    {
        printf("create shared mem!\n");
        *shm_fd = shm_open(SHM_NAME, (O_CREAT | O_RDWR), 0666);
        ftruncate(*shm_fd, sizeof(g_shared_mem));  // 공유 메모리의 사이즈를 설정한다.
        g_shared_mem =mmap(0, sizeof(shared_data), O_RDWR, MAP_SHARED, *shm_fd, 0);
        
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

        close(*shm_fd);

        printf("Open new shared mem, semaphore!\n");

        *shm_fd = shm_open(SHM_NAME, (O_CREAT | O_RDWR), 0666);
        ftruncate(*shm_fd, sizeof(g_shared_mem));  // 공유 메모리의 사이즈를 설정한다.
        g_shared_mem =mmap(0, sizeof(shared_data), O_RDWR, MAP_SHARED, *shm_fd, 0);
    }
}

static void run_user(int *shm_fd)
{
    char sem_name[255];

    char massage[256];
    *shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if(*shm_fd == -1)
    {
        perror("server is close! please create server\n");
        exit(1);
    }
    sem_t *user_sem;
    
    g_shared_mem =mmap(0, sizeof(shared_data), O_RDWR, MAP_SHARED, *shm_fd, 0);
    sprintf(sem_name, "user sem %d",g_shared_mem->recv_player);
    
    user_sem = sem_open(sem_name, O_CREAT, 0666, 1);
    if (user_sem == SEM_FAILED) 
    {
        switch (errno) 
        {
            case EEXIST:
                printf("close previous semaphore\n");
                sem_unlink(sem_name);
                user_sem = sem_open(sem_name, O_CREAT | O_EXCL, 0644, 1);
                break;
            case ENOENT:
                printf("Semaphore name is invalid.\n");
                break;
            case EACCES:
                printf("Insufficient permissions to create semaphore.\n");
                break;
            default:
                printf("Unknown error occurred.\n");
        }
    }
    
    // g_shared_mem->user_sem[g_shared_mem->recv_player] = user_sem;
    g_shared_mem->recv_player ++;
    strcpy(g_shared_mem->sem_name[g_shared_mem->recv_player], sem_name);
    while(1)
    {
        printf("send massage!\n");
        sprintf(massage,"task%d: hello!\n",g_shared_mem->recv_player);
        strcpy(g_shared_mem->data, massage);
        sleep(1);
        sem_post(user_sem);
    }
}