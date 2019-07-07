#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define NUM_THREADS 5
#define NUM_READ 10
#define NUM_WRITE 5

// void pointer to pass any type of argument
void *reader_read(void *ptr);
void *writer_write(void *ptr);

/**
 * mutexes
 */
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t read_phase = PTHREAD_COND_INITIALIZER;
pthread_cond_t write_phase = PTHREAD_COND_INITIALIZER;


/**
 * shared_var is shared by all the reader and writer
 * - multiple readers can read it at the same time
 * - only one writer can modify it at the same time
 */
int shared_var = 0;

/**
 * resource_counter to indicate the current resource
 * usage of shared_var.
 * = 0 : no reader or write is accessing resource
 * > 0 : there are one or more reader is accesing resource
 * = -1: one writer is using the resource
 */
int resource_counter = 0;

/**
 * read_times :
 * to count how many times reader has read the shared
 * variable. This variable is a shared variable for all
 * the readers.
 */
int read_times = 0;

/**
 * A shared variable for all the writers to record how many
 * times writers have made changes.
 */
int write_times = 0;

void *reader_read(void *ptr)
{
    int con = 1;
    int reader = *((int*)ptr);
    while (con)
    {
        pthread_mutex_lock(&counter_mutex);
        while (resource_counter < 0)
        {
            pthread_cond_wait(&read_phase, &counter_mutex);
        }
        resource_counter++;
        read_times++;
        pthread_mutex_unlock(&counter_mutex);

        // actual read
        if (read_times <=NUM_READ){
            printf("reader %ld read: shared_var->%d, read_times->%d\n", reader, shared_var, read_times);
        }else{
            con = 0;
        }

        pthread_mutex_lock(&counter_mutex);
        resource_counter--;
        if (resource_counter == 0){
            pthread_cond_signal(&write_phase);
        }
        pthread_mutex_unlock(&counter_mutex);

        pthread_cond_broadcast(&read_phase);

        int t = rand() % 5 + 1;
        printf("Now reader %ld will sleep for %d seconds.\n", reader, t);
        sleep(t);
        printf("Now reader %ld wakes up.\n",reader);
    }
    printf("reader %d terminate\n", reader);
}

void *writer_write(void *ptr)
{
    int writer = *((int*) ptr);
    printf("---------------%d",writer);
    while (write_times < NUM_WRITE)
    {
        pthread_mutex_lock(&counter_mutex);
        // IF some reader is reading -> wait
        while (resource_counter != 0)
        {
            pthread_cond_wait(&write_phase, &counter_mutex);
        }
        resource_counter--;
        pthread_mutex_unlock(&counter_mutex);

        // writer actions
        shared_var += 10;
        write_times++;
        printf("writer %d write: shared_var->%d, write_times->%d\n", writer, shared_var, write_times);

        // exit critical action
        pthread_mutex_lock(&counter_mutex);
        resource_counter++;
        pthread_cond_broadcast(&read_phase);
        pthread_cond_signal(&write_phase);
        pthread_mutex_unlock(&counter_mutex);

        int t = rand() % 5 + 1;
        printf("Now writer %d will sleep for %d seconds.\n", writer, t);
        sleep(t);
        printf("Now writer %d wakes up.\n",writer);
    }
    printf("writer %d terminates.\n", writer);
}

int main(void)
{
    int i;
    pthread_t thread_id[NUM_THREADS];

    for(i=0; i<NUM_THREADS;i++){
        if (i<3){
            pthread_create(&thread_id[i],NULL,reader_read,&i);
        }else{
            pthread_create(&thread_id[i],NULL,writer_write,&i);
        }

    }

    for(i=0; i<NUM_THREADS;i++){
        pthread_join(thread_id[i],NULL);
    }

    printf("Final shared variable value %d\n", shared_var);
    exit(0);
}