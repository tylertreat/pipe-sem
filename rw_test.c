#define _GNU_SOURCE

#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "pipe_sem.h"

#define NO_ACCESS 0
#define READER_ACCESS 1
#define WRITER_ACCESS 2

pipe_sem_t sem_reader;
pipe_sem_t sem_writer;
pipe_sem_t mutex;
int status = NO_ACCESS;
int count_reader_wait = 0;
int count_writer_wait = 0;
int count_reader_in_access = 0;

// The code run by a writer thread.
void writer(int id) {
    pipe_sem_wait(&mutex);
    if (status != NO_ACCESS) {
        // Reader(s) or writer accessing, so wait
        count_writer_wait++;
        pipe_sem_signal(&mutex);
        pipe_sem_wait(&sem_writer);
        pipe_sem_wait(&mutex);
        count_writer_wait--;
    }
    status = WRITER_ACCESS;
    pipe_sem_signal(&mutex);
    
    printf("Writer thread %d enters CS\n", id);
    sleep(1);
    printf("Writer thread %d is exiting CS\n", id);
    
    pipe_sem_wait(&mutex);
    if (count_writer_wait > 0) {
        // Signal waiting writer
        pipe_sem_signal(&sem_writer);
    } else if (count_reader_wait > 0) {
        // Signal waiting reader
        pipe_sem_signal(&sem_reader);
    } else {
        // No one is waiting
        status = NO_ACCESS;
    }
	pipe_sem_signal(&mutex);
}

// The code run by a reader thread.
void reader(int id) {
    pipe_sem_wait(&mutex);
    if (status == WRITER_ACCESS || count_writer_wait > 0) {
        // Writing or writer waiting, so wait
        count_reader_wait++;
        pipe_sem_signal(&mutex);
        pipe_sem_wait(&sem_reader);
        pipe_sem_wait(&mutex);
        count_reader_wait--;
        if (count_writer_wait == 0 && count_reader_wait > 0) {
            // Signal waiting readers if no writers waiting
            pipe_sem_signal(&sem_reader);
        }
    }
	status = READER_ACCESS;
	count_reader_in_access++;
	pipe_sem_signal(&mutex);
    
	printf("Reader thread %d enters CS\n", id);
    sleep(1);
    printf("Reader thread %d is exiting CS\n", id);
    
	pipe_sem_wait(&mutex);
	count_reader_in_access--;
	if (count_reader_in_access == 0) {
        // No more readers accessing
		if (count_writer_wait > 0) {
            // Signal waiting writers
            pipe_sem_signal(&sem_writer);
        } else {
            // No one is waiting
            status = NO_ACCESS;
        }
    }
	pipe_sem_signal(&mutex);
}

// Clean up semaphore resources.
void destroy_semaphore(pipe_sem_t *sem) {
    close(sem->fd[0]);
    close(sem->fd[1]);
}

int main(int argc, const char *argv[]) {
    
    // Parse input arguments
    if (argc < 3) {
        perror("Invalid number of arguments\n");
        perror("Usage: rw_test <number-of-arriving threads> <a sequence of 0 and 1 separated by a blank-space> <thread arrival interval>");
        return -1;
    }
    
    int thread_count = atoi(argv[1]);
    
    if (argc < 3 + thread_count) {
        perror("Invalid number of arguments\n");
        perror("Usage: rw_test <number-of-arriving threads> <a sequence of 0 and 1 separated by a blank-space> <thread arrival interval>");
        return -1;
    }
    
    int sequence[thread_count];
    int i;
    for (i = 0; i < thread_count; i++) {
        sequence[i] = atoi(argv[i + 2]);
    }
    
    int interval = atoi(argv[argc - 1]);
    
    pipe_sem_init(&sem_reader, 0);
    pipe_sem_init(&sem_writer, 0);
    pipe_sem_init(&mutex, 1);
    
    // Construct reader/writer threads
    pthread_t th[thread_count];
    for (i = 0; i < thread_count; i++) {
        pthread_create(&th[i], NULL, sequence[i] ? writer : reader, i);
        if (i < thread_count - 1) {
            sleep(interval);
        }
    }
    
    // Wait for the threads to terminate
    for (i = 0; i < thread_count; i++) {
        pthread_join(th[i], NULL);
    }
    
    // Clean up resources
    destroy_semaphore(&sem_reader);
    destroy_semaphore(&sem_writer);
    destroy_semaphore(&mutex);
    
    return 0;
}
