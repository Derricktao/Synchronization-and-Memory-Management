#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "queue.h"

typedef struct node {
	void* data;
	struct node* next;
}Node;

typedef struct queue {
	Node* front;
	Node* rear;
	int length;
}Queue;

queue_t queue_create(void)
{
	queue_t q = malloc(sizeof(Queue));
	q->length = 0;
	q->front = NULL;
	q->rear = NULL;
	return q;
}

int queue_destroy(queue_t queue)
{
	if (queue == NULL || queue->length > 0)
		return -1;
	if (queue->front)
		free(queue->front);
	if (queue->rear)
		free(queue->rear);
	free(queue);
	return 0;
}

int queue_enqueue(queue_t queue, void *data)
{
	if (queue == NULL || data == NULL)
		return -1;

	Node* temp = malloc(sizeof(Node));
	if (temp == NULL)
		return -1;

	queue->length++;

	temp->data = data;
	temp->next = NULL;
	if (queue->front == NULL && queue->rear == NULL){
		queue->front = temp;
		queue->rear = temp;
		return 0;
	}
	queue->rear->next = temp;
	queue->rear = temp;
	return 0;
}

int queue_dequeue(queue_t queue, void **data)
{

	if (queue == NULL || queue->length == 0 || data == NULL)
		return -1;

	if (queue->front == NULL){
		return -1;
	}

	if(queue!= NULL){
		*data = queue->front->data;
	}

	if (*data == NULL)
		return -1;

	if (queue->front == queue->rear){
		queue->front = NULL;
		queue->rear = NULL;
	}
	else{
		queue->front = queue->front->next;
	}

	queue->length--;
	return 0;
}

int queue_delete(queue_t queue, void *data)
{
	if (queue == NULL || data == NULL)
		return -1;

	if (queue->front == NULL)
		return -1;

	Node* temp = queue->front;
	Node* last = temp;
	int found_flag = 0;

	int i;
	for (i = 0; i < queue->length; i++){
		if (data == temp->data){
			if (i == 0)
				queue->front = temp->next;
			else if (i == queue->length-1){
                queue->rear = last;
                queue->rear->next = NULL;
            }
			else
				last->next = temp->next;

			free(temp);
			found_flag = 1;
			queue->length--;
			break;
		}
		last = temp;
		temp = temp->next;
	}

	if (queue->length == 0)
		queue->front = queue->rear = NULL;

	if (!found_flag){
		return -1;
	}
	else
		return 0;
}

int queue_iterate(queue_t queue, queue_func_t func, void *arg, void **data)
{
	if (queue == NULL || func == NULL)
		return -1;

	Node* temp = queue->front;

	int return_value = 0;
	int i;
	for (i = 0; i < queue->length; i++){
		if (data != NULL){
			*data = temp->data; 
		}
		return_value = func(queue, temp->data, arg);
		if(return_value == 1)
			return 0;
		temp = temp->next;
	}

	return 0;
}

int queue_length(queue_t queue)
{
	if (queue == NULL)
		return -1;
	else 
		return queue->length;
}

