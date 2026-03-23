#define BUF_SIZE 500
#define CMD_SIZE 500
#define OUTPUT_SIZE 500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stack.h"


#ifndef NULL
#define NULL 0
#endif


void push_back(struct cmd_queue* cs, struct cmd_queue** stack)
{
	while(stack[0] != NULL)
	{
		stack = &stack[0]->next;
	}

	stack[0] = cs;
}

void pop_back(struct cmd_queue** stack)
{
	struct cmd_queue * s = stack[0];
	if(stack[0] != NULL)
	{
		stack[0] = s->next;
		free(s);
	}
}

void clear_queue(struct cmd_queue** stack)
{
	struct cmd_queue *ptr = NULL;
	struct cmd_queue *prev = NULL;
	while(stack[0] != NULL)
	{
		prev = ptr;
		ptr = stack[0];
		stack[0] = NULL;
		stack = &ptr->next;
		free(prev);
	}
	free(ptr);
}


int queue_len(struct cmd_queue* s)
{
	int i = 0;
	while(s != NULL)
	{
		i = i + 1;
		s = s->next;
	}

	return i;
}

cmd_queue *make_queue(char* value)
{
	cmd_queue * tmp = malloc(sizeof(cmd_queue));
	memset(tmp, 0, sizeof(cmd_queue));
	strcpy(tmp->data,value);
	return tmp;
}

cmd_queue* first_in_queue(struct cmd_queue* stack)
{
	return stack;
}


