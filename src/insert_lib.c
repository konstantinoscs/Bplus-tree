#include "bf.h"
#include "file_info.h"
#include "stack.h"

extern file_info * openFiles[20];

int insert_index_val(void *value, int fileDesc, Stack** stack){
  BF_Block *curBlock;
  BF_Block_Init(&curBlock);
  //data will later store the block data to search
  char * data = NULL;
  //get the number of the bf file we're in
  bf_no = openFiles[fileDesc]->bf_desc;
  //get current block number from stack
  block_no = (*stack)->get_top();

  if(block_has_space()){
    //insert in current block
  }
  else if(is_root()){
    //split and update root
  }
  else{
    //pass it one level up
  }

  return 1;
}
