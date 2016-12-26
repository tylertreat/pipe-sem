#define _GNU_SOURCE

#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "pipe_sem.h"


static void release_lock(pipe_sem_t *sem);


// Initializes a semaphore and sets its initial value.
void pipe_sem_init(pipe_sem_t *sem, int value) {
    if (value < 0) {
        perror("Semaphore value must be greater than or equal to 0\n");
        exit(1);
    }
    sem->value = value;
    
    // Initialize the pipe
    if (pipe(sem->fd) != 0) {
        perror("Failed to initialize semaphore\n");
        exit(1);
    }
    
    release_lock(sem);
}

// Performs a wait operation on the semaphore.
void pipe_sem_wait(pipe_sem_t *sem) {
    while (sem->value == 0) {
        // Block the thread by reading from pipe
        char buff[10];
        read(sem->fd[0], buff, 10);
    }
    sem->value--;
}

// Performs a signal operation on the semaphore.
void pipe_sem_signal(pipe_sem_t *sem) {
    sem->value++;
    release_lock(sem);
}

static void release_lock(pipe_sem_t *sem) {
    int pid = fork();
    if (pid < 0) {
        perror("Failed to release lock\n");
        exit(1);
    }
    if (pid == 0) {
        // Release lock by writing to pipe
        write(sem->fd[1], "ok", 10);
        exit(0);
    }
}
