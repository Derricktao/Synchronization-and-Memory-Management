# ECS 150: Project #3 - Semaphores and TPS

In the project, we have created a version of our own semaphores and thread private storage. Through this projects, we have a
better understanding of synchronization and how to solve mutual exclusion. 

## The problems we encountered:

### Semaphore Implementation

Semaphore is just a generalized lock, with the help of queue, manage the threads that want to modify the data, so we have 
defined the following structs to help us implement the sem.c

```
typedef struct semaphore
{
	size_t count;
	queue_t waiting_queue;//typedef struct queue* queue_t; in queue.h
}*sem_t; //typedef struct semaphore *sem_t; in sem.h
```
The struct contains a queue as mentioned and a count.
As described in .h file, we implement ```sem_down, sem_up , sem_create and sem_destroy```. 
In our implementation, we initilize object sem_t together with the elements inside, and return the sem_t object.
Then there are two conditions inside ```sem_down/sem_up```: count is 0 and count is 1. If count is 1, we have to enqueue the 
current thread/dequeue the current thread in order to creating/dequeue a waiting threads queue for all threads that
are trying to modify the count, then block the thread; otherwise, just do -1/+1 instructions for count in sem_down/sem_up.
In the end, we call queue_destroy to free the queue we created and free sem_t object.
However, at first we use a while loop inside the ```sem_down``` (according to the lecture slide), which allowed us to pass
all the test, however, we found it wierld since after we delete all queue instrutions, it still works. 
Later we found out there are two problems: We kept the while loop thus it manually block itself when we should use thread_block
; we enter critical section before we modify the count, however we ignore the ```if(sem->count == 0)``` also acquires the 
enter of critical section. Thus we use enter_critical_section in the beginning and call exit_critical_section in the end of 
sem_down and sem_up. Since as long as we have to read/modify the count, we entered the critical section, we have to 
call enter_critical_section before read/modify. 

### TPS Implementation

TPS is another solution for mutual exclusion since every threads have its own memory space. To achieve the goal, we defined
the following structs to help us implement the tps.c

```
typedef struct{
	void* map;
	int reference_counter;
} PAGE;

typedef struct{
	pthread_t tid;
	PAGE* page; // size should be TPS_SIZE 4096
	int valid; // a boolean to distinguish between used & unused cell in array 
} TPS;

typedef struct{
	int init; // set to 1 if has been initialized already
	int index; // the index of the next free cell
	int size; // the current size of the array
	TPS* tps_array; // an array of TPS, enough for proof of concept O(size)
} TPS_BANK;

```

We created a ```TPS``` struct having tid number of current thread, PAGE object and a flag to distinguish between used & unused 
cell in array; a ```TPS_BANK``` struct having the size of current array, a flag to determine whether the initialization is 
completed, an ```TPS``` array; a ```PAGE``` struct having memory map pointer, and the reference_counter.
We initilize our tps by malloc an array, filled the array with mallloced page, then create page object by using malloc, then
we used mmap inside ```tps_create``` to map a page of memory to current thread. Also, if the array size is so full, we call
```expand_tps_array()``` to copy the existed array into a bigger array and free the privious small array.
Then we have a problem in excuting thread1 in ```tps.c``` test file. It print thread2 success followed by a segfault
We later found it is the our ```tps_clone``` and ```tps_destory``` function. We forgot to use ```munmap``` to free the memory
page we mapped, instead, we used ```free()```function; we forgot to call ```mprotect``` to change the privilege.

## Key points needed to state:

### sem.c:
  The key for this part is to understand how general lock works and how to use critical section.
  1. We call enter_critical_section as long as we read/modify ```sem->count```.
  2. We enqueue/dequeue the thread that wants to/already finish modify the count in ```sem_down```.
  3. We block/unblock the thread in sem_down and sem_up.
  
### tps.c:  
  The key for this part is to fully understand the difference between``` malloc``` and ```mmap```; ```free``` and ```munmap```.
  1. By using the three structs we declared above, we easily initilize them by creating an array, filled the array with page 
  objects, setting ```*map``` to null.
  2. We map a memory page for *map by using mmap and setting ```PROT_NONE``` and ```MAP_ANONYMOUS|MAT_PRIVATE``` since we won't
  want the page be accessed unless ```tps_read``` or ```tps_write``` is called. Also, by ```sem.h``` file, we have to use 
  copy-on-write mapping thus we use MAP_PRIVATE.
  3. In tps_read, we call mprotect to change our the priviledge of ```*map``` into ```PROT_READ``` and using ```memcpy``` to 
  copy it into buffer. After we finished ```memcpy```, we changed ```PROT_READ``` to ```PROT_NONE``` again.
  4. Each time we excute the ```tps_clone```, we call another function called ```tps_create_CoW``` to do Copy-on-Write 
  operation. Then we look for the target thread by calling ```find_target_TPS```to iterate through the whole ```tps_array```,
  then directly copy the ```page```object into our target ```page```, increment the ```reference_counter```.
  5. In tps_write, it is more complicated than tps_read. If the reference_counter is larger than 0, which means it is already
  clonde, we create ```old_page``` to hold the current page content, and then remmap a new memory page for ```*map```, memcopy
  the old_page into our newly mapped page. Otherwise, just use mprotect and memcpy, same as we did in tps_read.
  6. Added signal handler by following the instruction given in project prompt to monitor whether we have TPS protection
  error, making it easier to distinguish from seg fault.
  7. Since we used Array to store the all the tps pages, we have to expand the array as needed. Thus, in our ```expand_tps_array```
  We create a new array which has the double the size of previous one and then copy everything into the new array, then destroy
  the old array.
  8. Always use ```munmap``` to free the memory page, not ```free()```!


**useful resourse:**
http://www.cplusplus.com/reference/cstring/memcpy/
http://man7.org/linux/man-pages/man2/mmap.2.html
http://stackoverflow.com/questions/1739296/malloc-vs-mmap-in-c
http://stackoverflow.com/questions/8475609/implementing-your-own-malloc-free-with-mmap-and-munmap
