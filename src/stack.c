#include "stdlib.h"
#include "stack.h"

Stack * create_stack(){
 Stack * stack = malloc(sizeof(Stack));
 stack->size = 2;
 stack->keys = malloc(size*sizeof(int));
 stack->elems = 0;

 return stack;
}

int stack_push(Stack** stack, int value){
  //realloc stack if it has no available space
  if((*stack)->elems == (*stack)->size){
    (*stack)->size *= 2;
    *stack = realloc(*stack, (*stack)->size*sizeof(int));
  }
  (*stack)->keys[(*stack)->elems++] = value;
}

int stack_pop(Stack* stack){
  //if stack has 0 elements return -1
  if(!stack->elems)
    return -1;
  return stack->keys[--elems];
}

//same as above but don't decrement elems
int get_top(Stack* stack){
  if(!stack->elems)
    return -1;
  return stack->keys[elems-1];
}

int destroy_stack(Stack * stack){
  free(Stack);
}
