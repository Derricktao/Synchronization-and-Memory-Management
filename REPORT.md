# ECS 150: Project #3 - Semaphores and TPS

In the project, we have created a version of our own library that handles
semaphores and thread private storage. Through this projects, we have gained a
better understanding of synchronization and how to solve mutual exclusion.

### Semaphore Implementation

Semaphore is just a generalized lock that keeps a waiting list of the threads
who want to modify the data, with the help of a queue. We have defined the
following struct to help us implement the semaphore in `sem.c`.

```
typedef struct semaphore
{
	size_t count;
	queue_t waiting_queue;//typedef struct queue* queue_t; in queue.h
}*sem_t; //typedef struct semaphore *sem_t; in sem.h
```

The struct contains a queue which stores the thread ID of all the blocked
waiting threads and the count keeps track of the number of available resouce.

The queue starts in an empty state. Calling sem_up and sem_down functions will
increase and decrease the count by 1 accordingly. Once the count reaches 0,
meaning all the available resouces have been taken, all the following threads
that want to acquire the lock (by calling `sem_down()`) will be blocked and
their thread IDs will be put in the queue.

Whenever there are more resouce available again (for example, some thread calls
sem_up to increase the count), then the waiting threads in the queue will be
served (i.e. unblocked) in a FIFO order.

`sem_create and sem_destroy will be responsible for creating a new semaphore
`object and delete it when called. It's trival, so we will not discuss them
`here.

#### Problems Encountered

At first we used a busy waiting while loop based on count as our blocking method
inside the ```sem_down``` function,  which allowed us to pass all the test
without a hard fault. We didn't even keep check of any of the waiting threads
using the queue.

Later we found out there are two problems associated with our initial approach.
First, the busy waiting while loop really slows down the entire program down a
lot especially in the sem_prime test while there are many threads later in the
chain are waiting to be fed by the threads that are in the front of the chain.
Second problem was that we had no control over which waiting threads should be
served first without the queue.

So we started adding the queue implementation for waiting threads management.
But we spent some time on trouble shooting a racing condition after our library
became a little more complicated. It turned out that we omitted to make the read
operation for count as a critical session.

### Theard Private Storage Implementation

Theard Private Storage (TPS) is another solution for mutual exclusion. Since
every threads have its own privated address space, this implementaion make all
the threads mutually exclusive by its nature. To implement TPS, we have defined
the following structs in the tps.c

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

The PAGE struct represents the memory that we allocated for a thread. Each
thread will have it own PAGE unless it's still on its copy-on-write stage, in
which case, the thread will share the same PAGE with the thread that it copies
from. A single PAGE is possible to be shared by more than 2 threads. The
reference_count variable will keep track of how many threads are sharing the
current page.

TPS struct is one hierarchy above the PAGE struct. Each thread will have a TPS
struct associated with it. In this struct, we store the thread ID and a pointer
to the thread's PAGE. A `valid` boolean was used in additional to help us manage
the data structure we used.

TPS_BANK struct is the highest hierarch among the three. One running process
will have a copy of its own TPS_BANK. Basically, the TPS_BANK contains an array
of TPS structs. It keeps tracked of how big is the array, the index of the next
free cell in the array, and an `init` flag to prevent us from initializing the
same TPS_BANK twice. The best data structure to use should be a hash map here,
since we want to access the TPS struct by using the current thread ID as a
reference , in which case, seach time is O(1). Using an array, the search time
is O(n). However, this is just a proof-of-concept library. Since we are not
allowed to use external libraries for hash map, an array is simple enough to get
the job done. The `valid` boolean in PAGE was used to mark removed or invalid
TPS in the array so we can avoid the hassle of breaking the array from the
middle and shift the array in a remove operation.

An private function called expand_tps_array() will be called to double the size
of the array whenever the array is full.

## Key points needed to state:

### sem.c:

The key for this part is to understand how general lock works and how to use
critical section.

  1. We call enter_critical_section as long as we do read/write operations with
  ```sem->count```.

  2. We enqueue/dequeue the thread that wants to/already finish modify the count
  in ```sem_down```.

  3. We block/unblock the thread in sem_down and sem_up.
  
### tps.c:  

The key for this part is to fully understand the difference between``` malloc```
and ```mmap```; ```free``` and ```munmap```.

  1. By using the three structs we declared above, we easily initilize them by
  creating an array, filled the array with page  objects, setting ```*map``` to
  null.

  2. We map a memory page for *map by using mmap with ```PROT_NONE``` flag since
  we don't want the page be accessed unless ```tps_read``` or ```tps_write``` is
  called. We used ```MAP_ANONYMOUS|MAT_PRIVATE``` because `MAP_ANONYMOUS` cannot
  stay along. One of the MAP_SHARE and MAP_PRIVATE has to be set for the mmap to
  work. We chose MAP_PRIVATE in our implementation becuase we want each thread
  to have their own privated memory.

  3. In tps_read, we call mprotect to change the protection flag of ```*map```
  into ```PROT_READ``` and using ```memcpy``` to  copy it into buffer. After we
  finished ```memcpy```, we changed ```PROT_READ``` to ```PROT_NONE``` again.

  4. Each time we excute the ```tps_clone```, we call another function called
  ```tps_create_CoW``` to do Copy-on-Write create. A normal tps_create will
  allocate a new PAGE to the calling thread, but in the Copy-on-Write case, we
  only want the calling thread's PAGE pointer to point to an existing PAGE. To
  find the exiting PAGE address, we implemented ```find_target_TPS``` function
  that will help us to find the TPS struct of the target thread that we want to
  copy from in the `tps_array`. Then we point the current thread's PAGE pointer
  to the target thread's PAGE address, followed by incrementing the
  ```reference_counter``` of that existing PAGE.

  5. In tps_write, it is more complicated than tps_read. First, we have to check
  if it's a normal tps_write operation or a Copy-on-Write operation. If it's a
  normal wirte, we use mprotect to request for wirte permission by setting a
  `PROT_WRITE` flag, use memcpy to copy the content from the buffer to the
  thread's TPS, and use mprotect to reset the permission back to `PROT_NONE`
  after finishing writing. If the `reference_counter` is larger than 0, which
  means the current thread is waiting for a Copy-on-Write operation, we have to
  make a copy of the shared PAGE, decrease the `reference_counter` of the shared
  PAGE, and wirte the stuff in the buffer into the newly created PAGE.
  

  6. Added signal handler by following the instruction given in project prompt
  to monitor whether we have TPS protection error, making it easier to
  distinguish from a normal seg fault.

  7. Since we used Array to store the all the tps pages, we have to expand the
  array as needed. Thus, in our ```expand_tps_array``` We create a new array
  which has the double the size of previous one, copy every valid cells into
  the new array, then destroy the old array.

  8. Always use ```munmap``` to free the memory page, not ```free()```!


**useful resourse:**
http://www.cplusplus.com/reference/cstring/memcpy/

http://man7.org/linux/man-pages/man2/mmap.2.html

http://stackoverflow.com/questions/1739296/malloc-vs-mmap-in-c

http://stackoverflow.com/questions/8475609/implementing-your-own-malloc-free-
with-mmap-and-munmap

