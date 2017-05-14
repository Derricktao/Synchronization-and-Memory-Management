//UNFINISHED
#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "queue.h"
#include "thread.h"
#include "tps.h"

#define DEFAULT_SIZE 10
#define MAX_NUM_ALLOWED_TO_SHARE 100
/*
*	Private Section
*/

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

TPS_BANK tb; // global vairiable

/*
*	This function will be used to expand the tps_array when it's full
*	If malloc returns a NULL, function should return -1. If succeed, return 0
*/
int expand_tps_array(){
	/*
	* malloc an new array of double the size of the old array
	* copy the old array's content into the new allocated array
	* during the copy process, ignore the invalid cells created 
	* from tps_destroy();
	*/
	TPS* temp = tb.tps_array;
	tb.tps_array = malloc(tb.size*2*sizeof(TPS));

	tb.index = 0;
	for(int i =0; i < tb.size; i++){
		if (temp[i].valid){
			tb.tps_array[tb.index].tid = temp[i].tid;
			tb.tps_array[tb.index].page->map = temp[i].page->map;
			tb.tps_array[tb.index].valid = 1;
			tb.index++;
		}
	}
	free(temp);
	tb.size = tb.size*2;
	return 0;
}

/*
*	This function will return the index of the cell in array given 
*	the target_tid. It will return -1 if not found.
*/
int find_target_TPS(pthread_t target_tid){
	// Iterate through the tps_array and find the matching cell
	// return -1 if not found. Remember to check the valid boolean to 
	// see if it's a valid cell or not.
	for(int i =0; i < tb.size; i++){
		if(tb.tps_array[i].valid){
			if(target_tid == tb.tps_array[i].tid)
				return i;
		}
	}
	return -1;
}

static void segv_handler(int sig, siginfo_t *si, void *context)
{
    /*
     * Get the address corresponding to the beginning of the page where the
     * fault occurred
     */
    void *p_fault = (void*)((uintptr_t)si->si_addr & ~(TPS_SIZE - 1));
    /*
     * Iterate through all the TPS areas and find if p_fault matches one of them
     */
    int match = 0;
    for (int i =0; i < tb.index; i++){
    	if(tb.tps_array[i].page->map == p_fault){
    		match = 1;
    		break;
    	}
    }

    if (match)
        fprintf(stderr, "TPS protection error!\n");

    /* In any case, restore the default signal handlers */
    signal(SIGSEGV, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
    /* And transmit the signal again in order to cause the program to crash */
    raise(sig);
}

/*
int tps_create_CoW(void)
{
	pthread_t current_tid = pthread_self();
	if (find_target_TPS(current_tid)>=0)
		return -1; // return -1 if the current thread already has a TPS

	tb.tps_array[tb.index].tid = current_tid;
	tb.tps_array[tb.index].page->map = NULL;

	if (!tb.tps_array[tb.index].page->map || 
		tb.tps_array[tb.index].page->map == MAP_FAILED){
		perror("mmap");
		printf("map is fucked\n");
		return -1;
	}

	tb.tps_array[tb.index].valid = 1;

	tb.index++;
	if (tb.index == tb.size) // array is full
		if(expand_tps_array() == -1) // if expand fails (malloc fails)
			return -1;
	return 0;
}
*/

/*
*	Public Functions
*/

int tps_init(int segv)
{
	if (tb.init)
		return -1;
	if (segv) {
        struct sigaction sa;

        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = segv_handler;
        sigaction(SIGBUS, &sa, NULL);
        sigaction(SIGSEGV, &sa, NULL);
    }
	tb.init = 1;
	tb.index = 0;
	tb.size = DEFAULT_SIZE;
	tb.tps_array = malloc(DEFAULT_SIZE*sizeof(TPS));
	for(int i = 0; i < tb.size; i++){
		tb.tps_array[i].valid = 0;
		tb.tps_array[i].page = malloc(sizeof(PAGE));
		tb.tps_array[i].page->reference_counter = 0;
	}
	if (tb.tps_array == NULL)
		return -1;
	return 0;
}

int tps_create(void)
{
	pthread_t current_tid = pthread_self();
	if (find_target_TPS(current_tid)>=0)
		return -1; // return -1 if the current thread already has a TPS

	tb.tps_array[tb.index].tid = current_tid;
	tb.tps_array[tb.index].page = malloc(sizeof(PAGE));
	tb.tps_array[tb.index].page->map = mmap(
		NULL,
		TPS_SIZE,
		PROT_NONE,
		MAP_ANONYMOUS|MAP_PRIVATE,
		-1,
		0
	);

	if (!tb.tps_array[tb.index].page->map || 
		tb.tps_array[tb.index].page->map == MAP_FAILED){
		perror("mmap");
		return -1;
	}

	tb.tps_array[tb.index].valid = 1;

	tb.index++;
	if (tb.index == tb.size) // array is full
		if(expand_tps_array() == -1) // if expand fails (malloc fails)
			return -1;
	return 0;
}

int tps_destroy(void)
{
	int index;
	if ((index = find_target_TPS(pthread_self()))==-1)
		return -1; // Return -1 if current thread doesn't have a TPS.
	
	if(munmap(tb.tps_array[index].page->map,TPS_SIZE)){
		perror("munmap");
		exit(EXIT_FAILURE);
	}
	tb.tps_array[index].valid = 0; // Mark it as a invalid cell
	/* Note: remove from the middle of the array is expensive,
	* thus set it as invalid
	*/
	return 0;
}

int tps_read(size_t offset, size_t length, char *buffer)
{
	int index;
	if ((index = find_target_TPS(pthread_self()))==-1)
		return -1; // Return -1 if current thread doesn't have a TPS.
	if (TPS_SIZE-offset-length < 0)
		return -1; // reading operation is out of bound

	mprotect(tb.tps_array[index].page->map, TPS_SIZE, PROT_READ);
	memcpy(buffer, tb.tps_array[index].page->map + offset, length);
	mprotect(tb.tps_array[index].page->map, TPS_SIZE, PROT_NONE);
	return 0;
}

int tps_write(size_t offset, size_t length, char *buffer)
{
	int index;
	if ((index = find_target_TPS(pthread_self()))==-1)
		return -1; // Return -1 if current thread doesn't have a TPS
	if (TPS_SIZE-offset-length < 0)
		return -1; // reading operation is out of bound

	if (tb.tps_array[index].page->reference_counter > 0){
		void* old_map = tb.tps_array[index].page->map;
		tb.tps_array[index].page->map = mmap(
			NULL,
			TPS_SIZE,
			PROT_NONE,
			MAP_ANONYMOUS|MAP_PRIVATE,
			-1,
			0
		);
		mprotect(old_map, TPS_SIZE, PROT_READ);
		memcpy(buffer, old_map, TPS_SIZE);
		mprotect(old_map, TPS_SIZE, PROT_NONE);
		tb.tps_array[index].page->reference_counter--;
	}

	mprotect(tb.tps_array[index].page->map, TPS_SIZE, PROT_WRITE);
	memcpy(tb.tps_array[index].page->map + offset, buffer, length);
	mprotect(tb.tps_array[index].page->map, TPS_SIZE, PROT_NONE);
	return 0;
}

int tps_clone(pthread_t tid)
{
	// first phase: the cloned TPS's content(only) should be copied directly
	int sink_index, src_index;
	if ((sink_index = find_target_TPS(pthread_self()))>=0)
		return -1; // Return -1 if current thread already has a TPS
	if ((src_index = find_target_TPS(tid))==-1)
		return -1; // Return -1 if target thread doesn't have a TPS

	//tps_create_CoW();

	tps_create();

	sink_index = find_target_TPS(pthread_self());

	//tb.tps_array[sink_index].page->map = tb.tps_array[src_index].page->map;

	mprotect(tb.tps_array[sink_index].page->map, TPS_SIZE, PROT_WRITE);
	mprotect(tb.tps_array[src_index].page->map, TPS_SIZE, PROT_READ);
	memcpy(
		tb.tps_array[sink_index].page->map,
		tb.tps_array[src_index].page->map,
		TPS_SIZE
	);
	mprotect(tb.tps_array[sink_index].page->map, TPS_SIZE, PROT_NONE);
	mprotect(tb.tps_array[src_index].page->map, TPS_SIZE, PROT_NONE);

	return 0;
}

