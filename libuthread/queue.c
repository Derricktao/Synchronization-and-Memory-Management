#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "queue.h"

typedef struct Node{
    void *data;
    struct Node* next;
}ListNode;

struct queue {
    int len;
    ListNode *head;
    ListNode *tail;
};

typedef struct queue* que_ptr;

queue_t queue_create(void)
{
    que_ptr que = (struct queue *)malloc(sizeof(struct queue));

    if(que == NULL)
        return que;

    que -> len = 0;
    que -> head = que -> tail = NULL;
    return que;
}

int queue_destroy(queue_t queue)
{
    if(queue == NULL)
        return -1;
    else if(queue -> head == NULL || queue -> len == 0 )
        return -1;
    queue -> head = queue -> tail = NULL;
    queue -> len = 0;
    return 0;
}

int queue_enqueue(queue_t queue, void *data)
{
    ListNode *tmp = (ListNode*)malloc(sizeof(ListNode));
    if(queue == NULL)
      return -1;
    if(data == NULL)
        return -1;
    if(tmp == NULL)
        return -1;

    tmp -> next = NULL;
    tmp -> data = data;

    if(queue -> head == NULL){
        queue -> head = queue -> tail = tmp;
        queue -> len ++;
    }
    else{
        queue -> tail -> next = tmp;
        queue -> tail = tmp;
        queue -> len ++;
    }
    return 0;
}

int queue_dequeue(queue_t queue, void **data)
{
    if(queue -> head == NULL || queue -> len == 0){
        return -1;
    }

    *data = queue -> head -> data;
    ListNode *prev = queue -> head;
    queue -> head = queue -> head -> next;
    free(prev);
    queue -> len--;
    return 0;
}

int queue_delete(queue_t queue, void *data)
{
    if(queue -> head == NULL || queue -> len == 0){
        return -1;
    }

    ListNode *tmp, *prev;
    tmp = queue -> head;
    prev = NULL;
    while(tmp && tmp -> data != data){
        prev = tmp;
        tmp = tmp -> next;
    }

    if(tmp != NULL){//if tmp != NULL
        if(prev == NULL)// if prev != NULL
          queue -> head = tmp -> next;
        else// tmp is head
          prev -> next = tmp -> next;


        // free(tmp);
        queue -> len -= 1;
        return 0;
    }

    return -1;
}

int queue_iterate(queue_t queue, queue_func_t func, void *arg, void **data)
{
    if(func == NULL || queue -> len == 0 || queue == NULL)
        return -1;

    ListNode *tmp = queue -> head;
    int sign = 0;

    while(tmp != NULL){
        sign = func(queue, tmp->data, arg);

        if(sign != 0){
            *data = tmp -> data;
            return 0;
        }

        tmp = tmp -> next;
    }
    if(data != NULL) // might fix here
        *data = tmp -> data;
    return 0;
}

int queue_length(queue_t queue)
{
    return queue -> len;
}
