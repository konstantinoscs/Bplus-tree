#ifndef INSERT_LIB_H
#define INSERT_LIB_H

#include "stack.h"

int insert_index_val(void *value, int fileDesc, Stack* stack, int newbid);

int insert_leaf_val(void * value1, void* value2, int fileDesc, Stack * stack);

#endif
