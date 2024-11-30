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

#define SHM_NAME "shared memory"
#define SEM_MESSAGE_NAME "massage"

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
    char user_sem_name[MAX_USER][255];
    char massage_sem_name[255];
    char data[1024];
}shared_data;

static char temp[1024] = {0,};

volatile int key_pressed = 0;

static shared_data *g_shared_mem;

static sem_t *g_massage_sem;

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
static void *server_message_read_thread(void *shm);
static void write_message(void *shm, char *user_sem_name, char *massage);

/*----------------------------------------------------------------------------------------------------
**************************************** FUNCTION DEFINITIONS ****************************************
----------------------------------------------------------------------------------------------------*/

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
        g_shared_mem = mmap(0, sizeof(shared_data), O_RDWR, MAP_SHARED, *shm_fd, 0);


        strcpy(g_shared_mem->massage_sem_name, SEM_MESSAGE_NAME);
        g_massage_sem = sem_open(g_shared_mem->massage_sem_name, O_CREAT | O_EXCL, 0644, 1);
        
        // strcpy(g_shared_mem->user_sem_name[0], "test sem name");
        error = pthread_create(&thread, NULL, server_message_read_thread, (void *)g_shared_mem);
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
        sem_unlink(SEM_MESSAGE_NAME);
    }
}

static void run_user(int *shm_fd)
{
    char sem_name[255];
    char massage[256];
    u_int8_t user_num;
    *shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if(*shm_fd == -1)
    {
        perror("server is close! please create server\n");
        exit(1);
    }
    sem_t *user_sem;
    
    g_shared_mem =mmap(0, sizeof(shared_data), O_RDWR, MAP_SHARED, *shm_fd, 0);

    g_massage_sem = sem_open(g_shared_mem->massage_sem_name, 0);

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
    user_num = g_shared_mem->recv_player;
    strcpy(g_shared_mem->user_sem_name[user_num], sem_name);
    while(1)
    {
        sprintf(massage,"task%d: hello!\n",user_num);
        write_message(g_shared_mem, g_shared_mem->user_sem_name[user_num], massage);
        sleep(1);

        sem_post(user_sem);
    }
}

static void *server_message_read_thread(void *shm) 
{
    u_int8_t user_num = 1;
    shared_data *th_shm = (shared_data *)shm; 
    u_int32_t count = 0;
    printf("task start!\n");
    while(1) 
    {
        for(user_num = 1; user_num <= th_shm->recv_player; user_num ++)
        {
            if(th_shm->user_sem[user_num] == NULL)
            {
                th_shm->user_sem[user_num] = sem_open(th_shm->user_sem_name[user_num], 0);
            }
            printf("wait task%d![%d]\n",user_num,count);
            sem_wait(th_shm->user_sem[user_num]); // sempore 안오면 그냥 넘어감.
            printf("From: user%d: %s \n\n",user_num, th_shm->data);
            sem_post(g_massage_sem);
            count ++;
        }
        // sleep(1);
    }
}

static void write_message(void *shm, char *user_sem_name, char *massage)
{
    shared_data *shared_mem = (shared_data *)shm;
    sem_t * user_sem = sem_open(user_sem_name, 0);

    sem_wait(g_massage_sem);
    printf("send massage!\n");
    strcpy(shared_mem->data, massage);
    sem_post(user_sem);
}