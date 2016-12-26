#ifndef pipe_sem_h
#define pipe_sem_h

typedef struct {
    int fd[2];
    int value;
} pipe_sem_t;

// Initializes a semaphore and sets its initial value.
void pipe_sem_init(pipe_sem_t *sem, int value);

// Performs a wait operation on the semaphore.
void pipe_sem_wait(pipe_sem_t *sem);

// Performs a signal operation on the semaphore.
void pipe_sem_signal(pipe_sem_t *sem);

#endif
