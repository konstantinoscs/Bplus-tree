#include <string.h>
#include <stdlib.h>
#include "bf.h"
#include "file_info.h"
#include "stack.h"
#include "BoolType.h"
#include "AM.h"

extern file_info * openFiles[20];

//the declarations I will need, definitions are a the end of this file
int block_has_space(char * data, int keysize, int keys);
int find_offset(char* data, int keysize, void *key, int keytype);
//checks if we are in the root
int is_root(int block_no, int fileDesc);
//the standard initialization for an inside node
int initialize_block(char * data, int block_id);

//inserts an indexing value
int insert_index_val(void *value, int fileDesc, Stack** stack, int newbid){
  BF_Block *curBlock;
  BF_Block_Init(&curBlock);
  //get the number of the bf file we're in
  int bf_no = openFiles[fileDesc]->bf_desc;
  //get current block number from stack
  int block_no = get_top((*stack));
  //open block and get its data
  BF_GetBlock(bf_no, block_no, curBlock);
  char * data = BF_Block_GetData(curBlock);
  //get the number of keys because we may have to update the number later
  int offset = sizeof(bool) + 2*sizeof(int);
  int keys = 0;
  memmove(&keys, data+offset, sizeof(int));
  //add an int to offset to compute the total size
  offset += sizeof(int);
  //get necessary info about key
  int keytype = openFiles[fileDesc]->type1;
  int keysize =  openFiles[fileDesc]->length1;
  int total_size = offset + keys*keysize + (keys+1)*sizeof(int);

  if(block_has_space(data, keysize, keys)){
    //insert in current block
    offset = find_offset(data, keysize, value, keytype);
    //move existing data to the right and insert new data
    //bytes = cur_record + static -offset
    memmove(data+offset+keysize+sizeof(int), data+offset, total_size-offset);
    memmove(data+offset, value, keysize);
    offset += keysize;
    //here I should have which block is beneath
    memmove(data+offset, &newbid ,sizeof(int));
    //now set offset in the correct position to update the number of keys=
    offset = sizeof(bool) + 2*sizeof(int);
    keys++;
    memmove(data+offset, &keys, sizeof(int));
    //done :)
    BF_Block_SetDirty(curBlock);
  }
  else if(is_root(block_no, fileDesc)){
    //split and update root
    BF_Block *newBlock;
    BF_Block_Init(&newBlock);
    BF_AllocateBlock(bf_no, newBlock);
    int new_id = 0;
    BF_GetBlockCounter(bf_no, &new_id);
    new_id--;
    char * new_data = BF_Block_GetData(newBlock);
    initialize_block(new_data, new_id);
    //keys in 1st is ceil(key2+1/2)
    int keys_in1st = (keys+1)%2 ? (keys+1)/2 +1 : (keys+1)/2;
    int keys_in2nd = keys_in1st -1;
    //obviously in root there will be 1 key
    char * mid_key = malloc(keysize);
    //get key to up here
    //find middle key,
    //partition :)
    //workds also for the recursion
    //magic exists :)
    //copy half of the values
    //
    //update the root_id in openFiles
    //don't forget the metadata

    free(mid_key);
  }
  else{
    //split and pass one level up
    BF_Block *newBlock;
    BF_Block_Init(&newBlock);
  }

  //close/unpnin block ;)
  BF_Block_Destroy(&curBlock);

  return 1;
}

//preety straight forward
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
int is_root(int block_no, int fileDesc){
  return openFiles[fileDesc]->root_id == block_no;
}

//the standard initialization for an inside node
int initialize_block(char * data, int block_id){
  bool isLeaf = false;
  int nextPtr = -2;
  int recordsNum = 0;
  int offset = 0;
  memmove(data, &isLeaf, sizeof(bool));
  offset += sizeof(bool);
  memmove(data+offset, &block_id, sizeof(int));
  offset += sizeof(int);
  memmove(data+offset, &nextPtr, sizeof(int));
  offset += sizeof(int);
  memmove(data+offset, &recordsNum, sizeof(int));
  return 1;
}

int find_middle_key(char * data, char * mid_key, int keysize, int keys_in1st){
  int offset = sizeof(bool) + 3*sizeof(int);
  for(int i=0; i<keys_in1st; i++){
    //if()
  }
}
