#include "bf.h"
#include "file_info.h"
#include "stack.h"
//circular depndence
#include "AM.h"

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
  int keytype = openFiles[fileDesc]->type1;
  int keysize =  openFiles[fileDesc]->length1;

  if(block_has_space(data, keysize)){
    //insert in current block
    int offset = find_offset(data, keysize, value, keytype);
    //move existing data to the right and insert new data
    //memmove(data+offset+keysize+sizeof(int), data+offset, )
    memmove(data+offset, value, keysize);
    offset += keysize;
    //here I should have which block is beneath
    //memmove(data+offset, ,sizeof(int));
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
int find_offset(char* data, int keysize, void *key, int keytype){
 /*AM 1400192 STRONGLY OPPOSES the use of "bool"*/

 int offset = 0;
 int keys_no = 0;

 //add the first static data to offset
 offset += sizeof(bool) + 2*sizeof(int);
 //get the number of
 memmove(&keys_no, data+offset, sizeof(int));
 //add to the offset 4 bytes for keys_no and 4 for the first block pointer
 offset += 2*sizeof(int);

 //ATTENTION: this while should always finish since it's called always
 //when there is space in the block
 while(!keysComparer(key, data+offset, GREATER_THAN, keytype)){

   keys_no++;
   //move past one key plus the block pointer
   offset += keysize + sizeof(int);
 }
 //if the correct position was not found then it is the next one
 return offset;
}
