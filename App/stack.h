#ifndef STACK_H
#define STACK_H

typedef struct cmd_queue
{
	char data[BUF_SIZE];
	struct cmd_queue* next;
} cmd_queue;


void push_back(cmd_queue* cq, cmd_queue** stack);
void pop_back(cmd_queue** stack);
void clear_quque(cmd_queue** stack);
int stack_len(cmd_queue* stack);
cmd_queue *make_stack(char* value);
cmd_queue* first_in_stack(cmd_queue* stack);


#endif //!STACK_H
