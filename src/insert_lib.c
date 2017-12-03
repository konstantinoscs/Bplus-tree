#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bf.h"
#include "file_info.h"
#include "stack.h"
#include "BoolType.h"
#include "AM.h"
#include "HelperFunctions.h"

extern file_info * openFiles[20];
int size1;

void print_block(char * data){
  int ptrs = 0;
  int offset = 0;
  bool isleaf =0;
  int block_id = 0;
  int nextPtr=0;
  int recordsNum;
  int temp;
  char * val=malloc(size1);
  memmove(&isleaf, data, sizeof(bool));
  offset+= sizeof(bool);
  memmove(&block_id, data+offset, sizeof(int));
  offset+= sizeof(int);
  memmove(&nextPtr, data+offset, sizeof(int));
  offset += sizeof(int);
  memmove(&recordsNum, data+offset, sizeof(int));
  offset += sizeof(int);
  printf("Printing block with id %d\n", block_id);
  printf("Is leaf: %d\n", isleaf);
  printf("Nexptr %d\n", nextPtr);
  printf("recordsNum %d\n", recordsNum);
  // printf("p%d ", *((int *)data+offset));
  memmove(&temp, data +offset, sizeof(int));
  printf("p%d ", temp);
  offset+= sizeof(int);
  for(int i=0; i<recordsNum; i++){
    //printf("k%d ", *((int *)data+offset));
    memmove(val, data +offset, size1);
    printf("k%d ", temp);
    offset += sizeof(int);
    //printf("p%d ", *((int *)data+offset));
    memmove(&temp, data+offset, sizeof(int));
    printf("p%d ", temp);
    offset += sizeof(int);
  }
  printf("\n");
  free(val);
}

//the declarations I will need, definitions are a the end of this file
int block_has_space(char * data, int keysize, int keys);
int find_offset(char* data, int keysize, void *key, int keytype, int keys);
//checks if we are in the root
int is_root(int block_no, int fileDesc);
//the standard initialization for an inside node
int initialize_block(char * data, int block_id, int recs);

int find_middle_key(char * data, char * mid_key, void * key, int keytype,
  int keysize, int keys_in1st);

int partition(char *ldata, char *rdata, char * mid_key, void * key,
  int keytype, int keysize, int keys_in1st, int keys_in2nd, int offset,
  int newbid, int total_size);

bool leaf_block_has_space(int num_of_records, int len1, int len2);

//takes a records Attribute1 and finds the byte position where it should be
int findOffsetInLeaf(char* data, void *value, int fd);

int find_middle_record(char * data, char * mid_value1, void * value1, int type1,
  int size1, int size2, int recs_in1st);

int leaf_partition(char * ldata, char * rdata, void *mid_value, int type1,
  int size1, int size2, int total_records, void* value1, void* value2);

//inserts an indexing value (only for index blocks, never leaf)
int insert_index_val(void *value, int fileDesc, Stack* stack, int newbid){
  static int i =0;
  BF_Block *curBlock;
  BF_Block_Init(&curBlock);
  //get the number of the bf file we're in
  int bf_no = openFiles[fileDesc]->bf_desc;
  //get current block number from stack
  int block_no = get_top(stack);
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
  //the total size of the existing block
  int total_size = offset + keys*keysize + (keys+1)*sizeof(int);

  if(block_has_space(data, keysize, keys)){
    //insert in current block
    offset = find_offset(data, keysize, value, keytype, keys);
    //move existing data to the right and insert new data
    //bytes = cur_record + static -offset
    memmove(data+offset+keysize+sizeof(int), data+offset, total_size-offset);
    memmove(data+offset, value, keysize);
    offset += keysize;
    //here I should have which block is beneath
    memmove(data+offset, &newbid, sizeof(int));
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
    initialize_block(new_data, new_id, 0);
    //keys in 1st is ceil(key2+1/2)
    int keys_in1st = (keys+1)%2 ? (keys+1)/2 +1 : (keys+1)/2;
    int keys_in2nd = keys - keys_in1st;
    //obviously in root there will be 1 key
    char * mid_key = malloc(keysize);
    //get key to up here
    //find middle key and keep offset in left
    int offset_inl = find_middle_key(data, mid_key, value, keytype, keysize, keys_in1st);
    //partition :)

    partition(data, new_data, mid_key, value, keytype, keysize, keys_in1st,
      keys_in2nd, offset_inl, newbid, total_size);
    //create new root_id  //split and update root
    BF_Block *newroot;
    BF_Block_Init(&newroot);
    BF_AllocateBlock(bf_no, newroot);
    int root_id = 0;
    BF_GetBlockCounter(bf_no, &root_id);
    root_id--;
    char * root_data = BF_Block_GetData(newroot);
    initialize_block(root_data, root_id, 1);
    //offset is already sizeof(bool) + 3*sizeof(int) (arithmetic is ok here)
    memmove(root_data+offset, &block_no, sizeof(int));
    offset += sizeof(int);
    memmove(root_data+offset, mid_key, keysize);
    offset += keysize;
    memmove(root_data+offset, &new_id, sizeof(int));
    //update the root id in file_info
    openFiles[fileDesc]->root_id = root_id;
    //update the root_id in openFiles

    //cleanup
    BF_Block_SetDirty(curBlock);
    BF_Block_SetDirty(newBlock);
    BF_Block_SetDirty(newroot);
    BF_UnpinBlock(newBlock);
    BF_UnpinBlock(newroot);

    //now I will use newBlock to store the first block
    BF_GetBlock(bf_no, 0, newBlock);
    new_data = BF_Block_GetData(newBlock);
    //memmove the new root id now
    offset = 15 + 4*(sizeof(int));
    memmove(new_data+offset, &root_id, sizeof(int));
    //changed root if so set Dirty
    BF_Block_SetDirty(newBlock);
    BF_UnpinBlock(newBlock);
    BF_Block_Destroy(&newBlock);
    BF_Block_Destroy(&newroot);
    free(mid_key);
  }
  else{
    //split and pass one level up
    BF_Block *newBlock;
    BF_Block_Init(&newBlock);
    BF_AllocateBlock(bf_no, newBlock);
    int new_id = 0;
    BF_GetBlockCounter(bf_no, &new_id);
    new_id--;
    char * new_data = BF_Block_GetData(newBlock);
    initialize_block(new_data, new_id, 0);
    //keys in 1st is ceil(key2+1/2)
    int keys_in1st = (keys+1)%2 ? (keys+1)/2 +1 : (keys+1)/2;
    int keys_in2nd = keys - keys_in1st;
    //obviously in root there will be 1 key
    char * mid_key = malloc(keysize);
    //get key to up here
    //find middle key,
    int offset_inl = find_middle_key(data, mid_key, value, keytype, keysize, keys_in1st);
    //partition :)

    partition(data, new_data, mid_key, value, keytype, keysize, keys_in1st,
      keys_in2nd, offset_inl, newbid, total_size);

    //cleanup
    BF_Block_SetDirty(newBlock);
    BF_UnpinBlock(newBlock);
    BF_Block_Destroy(&newBlock);
    //pop stack and call recursion
    if(stack_pop((stack))== -1){
      printf("Invalid stack\n");
    }
    //recursive call
    insert_index_val(mid_key, fileDesc, stack, new_id);
    free(mid_key);
  }

  //close/unpnin block ;)
  BF_UnpinBlock(curBlock);
  BF_Block_Destroy(&curBlock);


  return 1;
}

//recursively adds a leaf val
int insert_leaf_val(void * value1, void* value2, int fileDesc, Stack * stack){
  //print_stack(stack);
  //printf("key to insert %d\n", *(int *)value);
  BF_Block *curBlock;
  BF_Block_Init(&curBlock);
  //get the number of the bf file we're in
  int bf_no = openFiles[fileDesc]->bf_desc;
  //get current block number from stack
  int block_no = get_top(stack);
  //open block and get its data
  BF_GetBlock(bf_no, block_no, curBlock);
  char * data = BF_Block_GetData(curBlock);
  bool isLeaf;
  memmove(&isLeaf, data, sizeof(bool));
  int offset = sizeof(bool)+2*sizeof(int);
  int records = 0;
  memmove(&records, data+offset, sizeof(int));
  offset += sizeof(int);
  int size1 = openFiles[fileDesc]->length1;
  int type1 = openFiles[fileDesc]->type1;

  //if is index block make the recursive call
  if(!isLeaf){
    //find the appropriate block id
    int block_id;
    //get this block id and push it to stack
    memmove(&block_id, data+sizeof(bool), sizeof(int));
    if (block_id != block_no){
      fprintf(stderr, "Wrong number in block id\n");
      exit(0);
    }
    //search current block to find the next block in down level
    //get data type for comparisons
    //pass the first block pointer
    offset+=sizeof(int);
    for(int i=0; i<records; i++){
      if(keysComparer(value1, data+offset, LESS_THAN, type1)){
        break;
      }
      offset += size1 + sizeof(int);
    }
    offset -= sizeof(int);
    memmove(&block_id, data+offset, sizeof(int));
    //unpin and destroy block here to save stack memory
    BF_UnpinBlock(curBlock);
    BF_Block_Destroy(&curBlock);
    stack_push(stack, block_id);
    return insert_leaf_val(value1, value2, fileDesc, stack);
  }
  int size2 = openFiles[fileDesc]->length2;
  int total_size = offset + records*(size1+size2);

  if(leaf_block_has_space(records, size1, size2)){
    offset = findOffsetInLeaf(data, value1, fileDesc);
    //move existing data to the right and insert new data
    memmove(data+offset+size1+size2, data+offset, total_size-offset);
    memmove(data+offset, value1, size1);
    offset += size1;
    memmove(data+offset, value2, size2);

    //now set offset in the correct position to update the number of records
    offset = sizeof(bool) + 2*sizeof(int);
    records++;
    memmove(data+offset, &records, sizeof(int));
    BF_Block_SetDirty(curBlock);
    BF_UnpinBlock(curBlock);
  }
  else{
    //split block and insert record
    BF_Block *newBlock;
    BF_Block_Init(&newBlock);
    BF_AllocateBlock(bf_no, newBlock);
    int new_id = 0;
    BF_GetBlockCounter(bf_no, &new_id);
    new_id--;
    char * new_data = BF_Block_GetData(newBlock);
    offset = sizeof(bool)+sizeof(int);
    int nextPtr;
    memmove(&nextPtr, data+offset, sizeof(int));
    memmove(data+offset, &new_id, sizeof(int));
    //initialize leaf block
    blockMetadataInit(new_data, 1, new_id, nextPtr, 0);

    int recs_in2nd = (records+1)%2 ? records/2 +1 : (records+1)/2;
    int recs_in1st = records +1 - recs_in2nd;
    char * mid_value = malloc(size1);
    find_middle_record(data, mid_value, value1, type1, size1, size2, recs_in1st);
    //partition with caution for same values
    leaf_partition(data, new_data, mid_value, type1, size1, size2, records+1,
      value1, value2);

    //set new records;
    //cleanup
    BF_Block_SetDirty(newBlock);
    BF_UnpinBlock(newBlock);
    BF_Block_SetDirty(curBlock);
    BF_UnpinBlock(curBlock);
    BF_Block_Destroy(&newBlock);
    stack_pop(stack);
    insert_index_val(mid_value, fileDesc, stack, new_id);

    free(mid_value);
  }
  BF_Block_Destroy(&curBlock);

}

//pretty straight forward
int block_has_space(char * data, int keysize, int keys){
  int avail_space = 0;
  //add the first static data to offset
  int offset = sizeof(bool) + 3*sizeof(int);
  //after the static offset we have (keys)*keysize + keys+1*int
  //and we want to add a key and an int
  avail_space = BF_BLOCK_SIZE - offset - (keys+1)*keysize - (keys+2)*sizeof(int);
  return avail_space < 0 ? 0 : 1;
}

//find offset works fine!!!
//find_offset takes a key and finds the byte position where it should be put
int find_offset(char* data, int keysize, void *key, int keytype, int keys){
 /*AM 1400192 STRONGLY OPPOSES the use of "bool"*/

 int offset = 0;
 int keys_no = 0;

 //add the first static data to offset
 offset += sizeof(bool) + 2*sizeof(int);
 //get the number of
 memmove(&keys_no, data+offset, sizeof(int));
 //add to the offset 4 bytes for keys_no and 4 for the first block pointer
 offset += 2*sizeof(int);
 if(keys!=keys_no)
  printf("Wrong keys\n");
 for(int i=0; i<keys; i++){
   if(keysComparer(key, data+offset, LESS_THAN, keytype))
    break;
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
int initialize_block(char * data, int block_id, int recs){
  bool isLeaf = false;
  int nextPtr = -2;
  int offset = 0;
  memmove(data, &isLeaf, sizeof(bool));
  offset += sizeof(bool);
  memmove(data+offset, &block_id, sizeof(int));
  offset += sizeof(int);
  memmove(data+offset, &nextPtr, sizeof(int));
  offset += sizeof(int);
  memmove(data+offset, &recs, sizeof(int));
  return 1;
}

//
int find_middle_key(char * data, char * mid_key, void * key, int keytype,
  int keysize, int keys_in1st){

  int offset = sizeof(bool) + 4*sizeof(int);
  int past_key = 0;
  for(int i=0; i<keys_in1st; i++){
    if(past_key || keysComparer(data+offset, key, LESS_THAN, keytype))
      offset += keysize +sizeof(int);
    else
      past_key = 1;
  }
  //copy the correct middle key to the variable
  if(past_key || keysComparer(data+offset, key, LESS_THAN, keytype)){
    memmove(mid_key, data+offset, keysize);
    offset += keysize +sizeof(int);
  }
  else{
    memmove(mid_key, key, keysize);
  }
  offset -= sizeof(int);
  //past this offset are the greater values
  return offset;
}


int partition(char *ldata, char *rdata, char * mid_key, void * key,
  int keytype, int keysize, int keys_in1st, int keys_in2nd, int offset,
  int newbid, int total_size){

  int roffset = sizeof(bool) + 2*sizeof(int);
  int loffset = offset;

  //move keys_in2nd no of keys in rblock and possibly insert the new key
  //first copy the number of keys
  memmove(rdata+roffset, &keys_in2nd, sizeof(int));
  roffset += sizeof(int);
  //then copy the first block pointer
  memmove(rdata+roffset, ldata+loffset, sizeof(int));
  loffset += sizeof(int);
  roffset += sizeof(int);
  //copy all the key/pointer pairs
  for(int i=0; i<keys_in2nd; i++){
    //copy the key
    if(loffset >=total_size || keysComparer(key, ldata+loffset, LESS_THAN, keytype)){
      memmove(rdata+roffset, key, keysize);
      roffset += keysize;
      memmove(rdata+roffset, &newbid, sizeof(int));
      roffset += sizeof(int);
    }
    else{
      memmove(rdata+roffset, ldata+loffset, keysize);
      loffset += keysize;
      roffset += keysize;
      //copy the block pointer
      memmove(rdata+roffset, ldata+loffset, sizeof(int));
      loffset += sizeof(int);
      roffset += sizeof(int);
    }
  }

  //keep keys_in1st no of keys in lblock and possibly insert the new key
  //first update the number of keys the left block will have
  loffset = sizeof(bool) + 2*sizeof(int);
  memmove(ldata+loffset, &keys_in1st, sizeof(int));
  loffset += 2*sizeof(int);

  //then start checking if we are ok or we should insert the new key
  for(int i=0; i<keys_in1st; i++){
    if(keysComparer(key, ldata+loffset, LESS_THAN, keytype)){
      //make newspace to put the key and the block pointer
      int newspace = keysize+sizeof(int);
      //memmove(ldata+loffset+newspace, ldata+loffset, total_size-offset-newspace);
      memmove(ldata+loffset+newspace, ldata+loffset, (keys_in1st-i-1)*newspace);
      //copy the key
      memmove(ldata+loffset, key, keysize);
      loffset += keysize;
      memmove(ldata+loffset, &newbid, sizeof(int));
      break;
    }
  }
  return 1;
  //done
}

bool leaf_block_has_space(int num_of_records, int len1, int len2){
  int new_used_space = sizeof(bool)+3*sizeof(int)+(num_of_records+1)*(len1+len2);
  int free_space = BF_BLOCK_SIZE - new_used_space;
  return free_space > 0 ? true : false;
}

//takes a records Attribute1 and finds the byte position where it should be
int findOffsetInLeaf(char* data, void *value, int fd){
  file_info* file = openFiles[fd];
  int num_of_records;
  memmove(&num_of_records,data+sizeof(bool)+2*sizeof(int),sizeof(int));
  //start from the record 0 of this block
  int offset = sizeof(bool)+3*sizeof(int);
  int curr_record = 0;
  //while value is GREATER_THAN the current record (find the first record that is GREATER_THAN_OR_EQUAL to value)
  while(keysComparer(value, data+offset, GREATER_THAN, file->type1) && curr_record<num_of_records){
    //skip current record because its smaller than value
    curr_record++;
    offset += file->length1+file->length2;
  }
  return offset;
}

int find_middle_record(char * data, char * mid_value1, void * value1, int type1,
  int size1, int size2, int recs_in1st){

    int offset = sizeof(bool) + 3*sizeof(int);
    int past_val1 = 0;
    for(int i=0; i<recs_in1st; i++){
      if(past_val1 || keysComparer(data+offset, value1, LESS_THAN, type1))
        offset += size1 + size2;
      else
        past_val1 = 1;
    }
    //copy the correct middle key to the variable
    if(past_val1 || keysComparer(data+offset, value1, LESS_THAN_OR_EQUAL, type1)){
      memmove(mid_value1, data+offset, size1);
      offset += size1 + size2;
    }
    else{
      memmove(mid_value1, value1, size1);
    }
    //restart offset in the blck
    offset = sizeof(bool) + 3*sizeof(int);

    //if middle value is equal to first find the next bigger
    if(keysComparer(data+offset, mid_value1, EQUAL, type1)){
      printf("Special case\n");
      while(keysComparer(data+offset, mid_value1, LESS_THAN_OR_EQUAL, type1)){
        offset += size1 + size2;

      }
      //copy the new mid value
      memmove(mid_value1, data+offset, size1);
    }
     return 1;
}

int leaf_partition(char * ldata, char * rdata, void *mid_value, int type1,
  int size1, int size2, int total_records, void* value1, void* value2){ //total_records=maxRecords+1

  int recs_in1st =0;
  int roffset=sizeof(bool)+3*sizeof(int);
  int loffset=sizeof(bool)+3*sizeof(int);
  int recs_for_2 = 0;
  int left_limit = loffset + (total_records-1)*(size1+size2);
  //pass everything LESS_THAN mid_value
  while(keysComparer(ldata+loffset,mid_value,LESS_THAN,type1)){
    loffset += size1+size2;
    recs_in1st++;
  }
  int recs_in2nd = total_records-recs_in1st;
  recs_for_2 = recs_in2nd;
  bool value1_is_set = false;
  //is the value1 going to the old or the new block?
  bool inNewBlock = true;
  if(keysComparer(value1, mid_value, LESS_THAN, type1)){ //if to the old
    recs_in2nd--;
    recs_for_2--;
    recs_in1st++;
    inNewBlock = false;
  }
  else if(keysComparer(value1, mid_value, EQUAL, type1)){  //if to the new but the value1=mid_value
    memmove(rdata+roffset,value1,size1);  //insert mid_value as the 1st record of new block
    roffset += size1;
    memmove(rdata+roffset, value2, size2);
    roffset += size2;
    recs_for_2--;
    value1_is_set= true;
  }
  // else if(keysComparer(value1,ldata+loffset,EQUAL,type1) && !value1_is_set){
  //   memmove(rdata+roffset, value1, size1);
  //   roffset += size1;
  //   memmove(rdata+roffset, value2, size2);
  //   roffset += size2;
  //   value1_is_set = true;
  // }
  for(int i=0; i<recs_for_2; i++){
    //check if this is where the value1 will go
    if(loffset>=left_limit){
      memmove(rdata+roffset, value1, size1);
      roffset += size1;
      memmove(rdata+roffset, value2, size2);
      roffset += size2;
      value1_is_set = true;
    }
    else if((keysComparer(value1,ldata+loffset,EQUAL,type1) && !value1_is_set)){
      memmove(rdata+roffset, value1, size1);
      roffset += size1;
      memmove(rdata+roffset, value2, size2);
      roffset += size2;
      value1_is_set = true;
    }
    else if(keysComparer(value1,mid_value,GREATER_THAN,type1) && keysComparer(value1,ldata+loffset,LESS_THAN,type1)&& !value1_is_set){
      memmove(rdata+roffset, value1, size1);
      roffset += size1;
      memmove(rdata+roffset, value2, size2);
      roffset += size2;
      value1_is_set = true;
    }
    else{ //else value1 is not here, but an old record is
      memmove(rdata+roffset, ldata+loffset,size1);
      loffset += size1;
      roffset += size1;
      memmove(rdata+roffset,ldata+loffset,size2);
      loffset += size2;
      roffset += size2;
    }
  }
  //now the 2nd block is in the goal state
  if(!value1_is_set){ //check if value1 goes to the old block
    loffset = sizeof(bool)+3*sizeof(int);
    for(int i=0; i<recs_in1st; i++){
      if(keysComparer(value1,ldata+loffset,LESS_THAN,type1)){ //this is where value1 will go
        //move the data from here to the right to make space for value1
        int newspace = size1+size2;
        int recs_left_in1st = recs_in1st-1-i;
        memmove(ldata+loffset+newspace, ldata+loffset, recs_left_in1st*(size1+size2));
        //insert value1 and value2
        memmove(ldata+loffset,value1,size1);
        loffset += size1;
        memmove(ldata+loffset,value2,size2);
        loffset += size2;
      }
      else{ //value1 isnt going here, lets look at the next record
        loffset += size1+size2;
      }
    }
  }
  //now the 1st block is in the goal state and the value1 is inserted
  //update recordsNum in old and new block
  memmove(ldata+sizeof(bool)+2*sizeof(int), &recs_in1st, sizeof(int));
  memmove(rdata+sizeof(bool)+2*sizeof(int), &recs_in2nd, sizeof(int));
  return 1;
}

/*2
222256

22222 56

2
1 22222

1 222222

5

22222 5*/
