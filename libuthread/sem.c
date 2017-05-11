//UNFINISHED
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include "queue.h"
#include "sem.h"
#include "thread.h"

typedef struct semaphore
{
	size_t count;//typedef struct semaphore *sem_t; in sem.h
	queue_t waiting_queue;//typedef struct queue* queue_t; in queue.h
}*sem_t;

sem_t sem_create(size_t count)
{
	sem_t sem = malloc(sizeof(sem_t));
	if(!sem){return NULL;}
	sem->count = count;
	sem->waiting_queue = queue_create();
	return sem;
}

int sem_destroy(sem_t sem)
{
	if(!sem||!sem->waiting_queue){return -1;}
	queue_destroy(sem->waiting_queue);
	free(sem);
	return 0;
}

int sem_down(sem_t sem)
{
	if((sem->count == 0) && (queue_length(sem->waiting_queue) > 0)){
		struct pthread* temp;
		queue_enqueue(sem->waiting_queue, (void**) &temp);
		thread_block();
	}
	if(!sem||!sem->waiting_queue){return -1;}
	while(sem->count == 0){/*block itself*/}
	sem->count -= 1;
	return 0;
}

int sem_up(sem_t sem)
{
	if((sem->count == 0) && (queue_length(sem->waiting_queue) > 0)){
		struct pthread* temp;
		queue_dequeue(sem->waiting_queue, (void**) &temp);
		thread_block();
	}
	if(!sem||!sem->waiting_queue){return -1;}
	enter_critical_section();
	sem->count += 1;
	exit_critical_section();
	return 0;
}
