#include "bf.h"
#include "file_info.h"
#include "stack.h"
//circular depndence
#include "AM.h"

extern file_info * openFiles[20];

//the declarations I will need, definitions are a the end of this file
int find_offset(char* data, int keysize, void *key, int keytype);
//checks if we are in the root
int is_root(int block_no. int fileDesc);

//inserts an indexing value
int insert_index_val(void *value, int fileDesc, Stack** stack, int newbid){
  BF_Block *curBlock;
  BF_Block_Init(&curBlock);
  //get the number of the bf file we're in
  int bf_no = openFiles[fileDesc]->bf_desc;
  //get current block number from stack
  int block_no = (*stack)->get_top();
  //open block and get its data
  BF_GetBlock(bf_no, block_no, curBlock);
  char * data = BF_Block_GetData(curBlock);
  //get the number of keys because we may have to update the number later
  int offset = sizeof(bool) + 2*sizeof(int);
  memmove(&keys, data+offset, sizeof(int));
  //add an int to offset to compute the total size
  offset += sizeof(int);
  //get necessary info about key
  int keytype = openFiles[fileDesc]->type1;
  int keysize =  openFiles[fileDesc]->length1;
  int total_size = offset + keys*keysize + (keys+1)*sizeof(int);

  if(total_size = block_has_space(data, keysize)){
    //insert in current block
    offset = find_offset(data, keysize, value, keytype);
    //move existing data to the right and insert new data
    //bytes = cur_record + static -offset
    memmove(data+offset+keysize+sizeof(int), data+offset, total_size-offset);
    memmove(data+offset, value, keysize);
    offset += keysize;
    //here I should have which block is beneath
    memmove(data+offset, newbid ,sizeof(int));
    //now set offset in the correct position to update the number of keys=
    offset = sizeof(bool) + 2*sizeof(int);
    memmove(data+offset, &++keys, sizeof(int));
    //done :)
    BF_Block_SetDirty(curBlock);
  }
  else if(is_root(block_no, fileDesc)){
    //split and update root
  }
  else{
    //split and pass one level up
  }

  //close/unpnin block ;)
  BF_Block_Destroy(&curBlock);

  return 1;
}

//block_has space will return the size of block if it's enough :)
int block_has_space(char * data, int keysize, int keys){
  int avail_space = BF_BLOCK_SIZE;
  //add the first static data to offset
  int offset = sizeof(bool) + 3*sizeof(int);
  //after the static offset we have (keys)*keysize + keys+1*int
  //and we want to add a key and an int
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

//checks if we are in the root
int is_root(int block_no. int fileDesc){
  return openFiles[fileDesc]->root_id == block_no;
}
