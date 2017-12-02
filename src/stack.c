#include <stdio.h>
#include "stdlib.h"
#include "stack.h"

int create_stack(Stack** stack){
  *stack = malloc(sizeof(Stack));
  (*stack)->size = 2;
  (*stack)->keys = malloc((*stack)->size*sizeof(int));
  (*stack)->elems = 0;

  return 1;
}

int stack_push(Stack* stack, int value){
  //realloc stack if it has no available space
  if(stack->elems == stack->size){
    stack->size *= 2;
    stack->keys = realloc(stack->keys, stack->size*sizeof(int));
  }
  stack->keys[stack->elems++] = value;
}

int stack_pop(Stack* stack){
  //if stack has 0 elements return -1
  if(!stack->elems)
    return -1;
  return stack->keys[--stack->elems];
}

//same as above but don't decrement elems
int get_top(Stack* stack){
  if(!stack->elems)
    return -1;
  return stack->keys[stack->elems-1];
}

int destroy_stack(Stack * stack){
  free(stack->keys);
  free(stack);
}

void print_stack(Stack * stack){
  for(int i=0; i<stack->elems; i++)
    printf("Stack[%d] = %d\n", i,stack->keys[i]);
}
