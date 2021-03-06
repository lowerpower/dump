#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "queue.h"

#define QUEUE_POISON1 ((void*)0xCAFEBAB5)

struct queue_root {
	struct queue_head *head;
	pthread_mutex_t head_lock;

	struct queue_head *tail;
	pthread_mutex_t tail_lock;

	struct queue_head divider;
};

struct queue_root *ALLOC_QUEUE_ROOT()
{
	struct queue_root *root = \
		malloc(sizeof(struct queue_root));
	pthread_mutex_init(&root->head_lock, NULL);
	pthread_mutex_init(&root->tail_lock, NULL);

	root->divider.next = NULL;
	root->head = &root->divider;
	root->tail = &root->divider;
	return root;
}

void INIT_QUEUE_HEAD(struct queue_head *head)
{
	head->next = QUEUE_POISON1;
}

void queue_put(struct queue_head *new,
	       struct queue_root *root)
{
	new->next = NULL;

	pthread_mutex_lock(&root->tail_lock);
	root->tail->next = new;
	root->tail = new;
	pthread_mutex_unlock(&root->tail_lock);
}

struct queue_head *queue_get(struct queue_root *root)
{
	struct queue_head *head, *next;

	while (1) {
		pthread_mutex_lock(&root->head_lock);
		head = root->head;
		next = head->next;
		if (next == NULL) {
			pthread_mutex_unlock(&root->head_lock);
			return NULL;
		}
		root->head = next;
		pthread_mutex_unlock(&root->head_lock);
		
		if (head == &root->divider) {
			queue_put(head, root);
			continue;
		}

		head->next = QUEUE_POISON1;
		return head;
	}
}
