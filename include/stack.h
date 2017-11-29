#ifndef STACK_H
#define STACK_H

typedef struct Stack{
  void * key;
  int key_size;
  int size;
}stack;

//takes a stack pointer (not malloc'd) and the size of the key
//and creates the stack
int create_stack(stack * Stack, int key_size);

//pushes value in the stack
int stack_push(void * value);

//takes a malloc'd value and pops in it
int stack_pop(void * value);

//takes a stack pointer and destroys the corresponding stack
int destroy_stack(stack * Stack);

#endif
