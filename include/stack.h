#ifndef STACK_H
#define STACK_H

typedef struct Stack{
  int size;
  int * keys;
  int elems;
}Stack;

//creates a stack and returns a pointer to it
Stack * create_stack();

//pushes value in the stack
int stack_push(Stack** stack, int value);

//pops from stack
//returns -1 if stack is empty
int stack_pop(Stack* stack){

//takes a stack pointer and destroys the corresponding stack
int destroy_stack(Stack * stack);

#endif
