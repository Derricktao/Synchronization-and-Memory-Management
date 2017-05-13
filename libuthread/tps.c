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

/*
*	Private Section
*/

typedef struct{
	pthread_t tid;
	void* map; // size should be TPS_SIZE 4096
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
	/* TO DO:
	* malloc an new array of double the size of the old array
	* copy the old array's content into the new allocated array
	* during the copy process, ignore the invalid cells created 
	* from tps_destroy();
	*/
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
/*
*	Public Functions
*/

int tps_init(int segv)
{
	if (tb.init)
		return -1;
	tb.init = 1;
	tb.index = 0;
	tb.size = DEFAULT_SIZE;
	tb.tps_array = malloc(DEFAULT_SIZE*sizeof(TPS));
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
	//tb.tps_array[tb.index].map = mmap(); //TO DO: need to check the API	
	// place holder: just malloc should be able to pass phase 1 as well
	tb.tps_array[tb.index].map = malloc(TPS_SIZE);

	if (tb.tps_array[tb.index].map == NULL)
		return -1;

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
	free(tb.tps_array[index].map);
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
	memcpy(buffer, tb.tps_array[index].map + offset, length);
	return 0;
}

int tps_write(size_t offset, size_t length, char *buffer)
{
	int index;
	if ((index = find_target_TPS(pthread_self()))==-1)
		return -1; // Return -1 if current thread doesn't have a TPS
	if (TPS_SIZE-offset-length < 0)
		return -1; // reading operation is out of bound
	memcpy(tb.tps_array[index].map + offset, buffer, length);
	return 0;
}

int tps_clone(pthread_t tid)
{
	printf("tps_clone, tps_array size is %d\n", tb.size);
	// first phase: the cloned TPS's content(only) should be copied directly
	int sink_index, src_index;
	if ((sink_index = find_target_TPS(pthread_self()))>=0)
		return -1; // Return -1 if current thread already has a TPS
	if ((src_index = find_target_TPS(tid))==-1)
		return -1; // Return -1 if target thread doesn't have a TPS

	tps_create();

	sink_index = find_target_TPS(pthread_self());

	printf("starting clone from index %d to index %d \n", sink_index, src_index);

	memcpy(
		tb.tps_array[sink_index].map,
		tb.tps_array[src_index].map,
		TPS_SIZE
	);
	return 0;
}

