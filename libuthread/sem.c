//UNFINISHED
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include "queue.h"
#include "sem.h"
#include "thread.h"

typedef struct semaphore
{
	size_t count;
	queue_t waiting_queue;
}*sem_t;
//typedef struct semaphore *sem_t; defined in .h

sem_t sem_create(size_t count)
{
	sem_t sem = malloc(sizeof(sem_t));
	sem->count = count;
	sem->waiting_queue = queue_create();
	return sem;
}

int sem_destroy(sem_t sem)
{
	if(!sem||!sem->waiting_queue)
	{
		return -1;
	}
	else
	{
		free(sem->waiting_queue);
		free(sem);
	}
	return 0;
}

int sem_down(sem_t sem)
{
	while(sem->count == 0){/*block itself*/}
	sem->count -= 1;
	return 0;
}

int sem_up(sem_t sem)
{
	sem->count += 1;
	return 0;
}
