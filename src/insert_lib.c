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
  int bf_no = openFiles[fileDesc]->bf_desc;
  //get current block number from stack
  int block_no = (*stack)->get_top();
  int keysize =  openFiles[fileDesc]->length1;

  if(block_has_space(data, keysize)){
    //insert in current block
  }
  else if(is_root()){
    //split and update root
  }
  else{
    //split and pass one level up
  }

  return 1;
}

int block_has_space(char * data, int keysize){
  int keys;
  int avail_space = BF_BLOCK_SIZE;
  //add the first static data to offset
  int offset = sizeof(bool) + 2*sizeof(int);
  memmove(&keys, data+offset, sizeof(int));
  offset +=sizeof(int);
  avail_space -= offset - (keys+1)*keysize - (keys+2)*sizeof(int);
  return avail_space < 0 ? 0 : 1;
}

//find_offset takes a key and finds the byte position where it should be put
int find_offset(char* data, int keysize, void *key){
 /*AM 1400192 STRONGLY OPPOSES the use of "bool"*/

 int offset = 0;
 int key_no = 0;

 //add the first static data to offset
 offset += sizeof(bool) + 3*sizeof(int);
 //the number of records is 9 bytes after the start of the block
 memmove(&records_no, data+9, sizeof(int));
 //iterate for each key/block pointer pair
 //ATTENTION: this while should always finish since
 while(record < records_no){
   //if the key we just got to is greater or equal to the key we are looking for
   //return this position - its either the position it is or it should be
   if (keysComparer(data+offset, value1, GREATER_THAN_OR_EQUAL, type))
   {
     return record;
   }
   record++;
   offset += len1 + len2;
 }
 //if the correct position was not found then it is the next one
 return record;
}
