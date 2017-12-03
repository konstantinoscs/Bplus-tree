#include "HelperFunctions.h"

/***************************************************************************************************************************
**************INSERT AND SCAN*********************************************************************************************************
******************************************************************************************************************************/

//Takes the search key(target key) and the traversing key(tmp key), the operation (EQUAL, LESS_THAN etc) that we want to apply on the keys and the keytype
//and returns if the operation is true or false
//is targetkey op tmpkey?
bool keysComparer(void *targetKey, void *tmpKey, int operation, int keyType){
  int len = strlen((char*)tmpKey);
  char* str = malloc(len+1);
  memmove(str,targetKey,len);
  str += '\0';

  switch (operation){
    case EQUAL:
      switch (keyType){
        case 1:
          if (*(int *)targetKey == *(int *)tmpKey)
            return 1;
          return 0;
        case 2:
          if (*(float *)targetKey == *(float *)tmpKey)
            return 1;
          return 0;
        case 3:
        printf("Comparing %s vs %s\n", (char*)targetKey,(char*)tmpKey);
          if (!strncmp((char *)targetKey, (char *)tmpKey, len)) //When comparing strings we dont need to know their length
                                                          //because strcmp stops to the null term char
          {
            printf("SAME\n");
            return 1;
          }
          printf("NOT SAME\n");
          return 0;
      }
    case NOT_EQUAL:
      switch (keyType){
        case 1:
          if (*(int *)targetKey != *(int *)tmpKey)
            return 1;
          return 0;
        case 2:
          if (*(float *)targetKey != *(float *)tmpKey)
            return 1;
          return 0;
        case 3:
          if (strcmp((char *)targetKey, (char *)tmpKey))
            return 1;
          return 0;
      }
    case LESS_THAN:
      switch (keyType){
        case 1:
          if (*(int *)targetKey < *(int *)tmpKey)
            return 1;
          return 0;
        case 2:
          if (*(float *)targetKey < *(float *)tmpKey)
            return 1;
          return 0;
        case 3:
          if (strcmp((char *)targetKey, (char *)tmpKey) < 0)
            return 1;
          return 0;
      }
    case GREATER_THAN:
      switch (keyType){
        case 1:
          if (*(int *)targetKey > *(int *)tmpKey)
            return 1;
          return 0;
        case 2:
          if (*(float *)targetKey > *(float *)tmpKey)
            return 1;
          return 0;
        case 3:
          if (strcmp((char *)targetKey, (char *)tmpKey) > 0)
            return 1;
          return 0;
      }
    case LESS_THAN_OR_EQUAL:
      switch (keyType){
        case 1:
          if (*(int *)targetKey <= *(int *)tmpKey)
            return 1;
          return 0;
        case 2:
          if (*(float *)targetKey <= *(float *)tmpKey)
            return 1;
          return 0;
        case 3:
          if (strcmp((char *)targetKey, (char *)tmpKey) <= 0)
            return 1;
          return 0;
      }
    case GREATER_THAN_OR_EQUAL:
      switch (keyType){
        case 1:
          if (*(int *)targetKey >= *(int *)tmpKey)
            return 1;
          return 0;
        case 2:
          if (*(float *)targetKey >= *(float *)tmpKey)
            return 1;
          return 0;
        case 3:
          if (strcmp((char *)targetKey, (char *)tmpKey) >= 0)
            return 1;
          return 0;
      }
  }
  free(str);
}

//Returning the stack full with the path to the leaf and the id of the leaf
//that has the key we are looking for or should have it at least. If called with null leafPath we didnt need
//to keep tha path in this function call
int findLeaf(int fd, void *key, Stack *leafPath){
  BF_Block *tmpBlock;
  BF_Block_Init(&tmpBlock);

  int keyType, keyLength, rootId, tmpBlockPtr, keysNumber, targetBlockId, tmpBlockId;
  void *data;
  char *tmpKey = malloc(openFiles[fd]->length1);
  bool isLeaf = 0;

  //Get the type and the length of this file's key and the root
  keyType = openFiles[fd]->type1;
  keyLength = openFiles[fd]->length1;
  rootId = openFiles[fd]->root_id;

  CALL_OR_DIE(BF_GetBlock(openFiles[fd]->bf_desc, rootId, tmpBlock)); //Get the root block to start searching
  data = BF_Block_GetData(tmpBlock);

  memcpy(&isLeaf, data, sizeof(bool));

  if (openFiles[fd]->rootInitialized == 0) //If the root is not initialized so no insertion is done before
  {
    int offset = sizeof(bool) + sizeof(int)*4 + keyLength;
    memcpy(&targetBlockId, data + offset, sizeof(int));
    CALL_OR_DIE(BF_UnpinBlock(tmpBlock));
    BF_Block_Destroy(&tmpBlock);
    return targetBlockId;
  }

  while( isLeaf == 0){  //Everytime we get in a new block to search that is not a leaf

    int currKey = 0;  //Initialize the index of keys pointer

    data += sizeof(char); //Move the data pointer over the isLeaf byte

    memcpy(&tmpBlockId, data, sizeof(int)); //Take the id of the block we are currently checking

    if (leafPath != NULL) //In case it is null it means that in this function call we didnt need to keep the path too
    {
      stack_push(leafPath, tmpBlockId);  //And push it to the leaf path
    }

    data += sizeof(int)*2;  //Move the data pointer over the block id and the next block pointer they are useless now

    memcpy(&keysNumber, data, sizeof(int)); //Get the number of the keys that exist in this block
    data += sizeof(int);
    memcpy(&tmpBlockPtr, data, sizeof(int));  // Get the first pointer to child block that exist in this block
    data += sizeof(int);
    memcpy(tmpKey, data, openFiles[fd]->length1); //Get the value of the first key in this block
    data += openFiles[fd]->length1;
    currKey++;  //Increase the index pointer

    while(keysComparer(key, tmpKey, GREATER_THAN_OR_EQUAL, keyType) && currKey < keysNumber){ //while the key that we look for is bigger than the key that we have now
      //and the index number is smaller than the amount of keys in this block (so we are not on the last key) keep traversing
      memcpy(&tmpBlockPtr, data, sizeof(int));  //get the pointer to the child block that is before the new tmpKey
      data += sizeof(int);
      memcpy(tmpKey, data, openFiles[fd]->length1); //get the new tmp key
      data += openFiles[fd]->length1;
      currKey++;
    }

    if (keysComparer(key, tmpKey, GREATER_THAN_OR_EQUAL, keyType))  //if the loop stopped because we reached the last key on this block but still the key
    //that we are looking for is bigger than the last key
    {
      memcpy(&tmpBlockPtr, data, sizeof(int));  //Then get the last child pointer of this block
      CALL_OR_DIE(BF_UnpinBlock(tmpBlock));

      CALL_OR_DIE(BF_GetBlock(openFiles[fd]->bf_desc, tmpBlockPtr, tmpBlock));  //Get to this block
      data = BF_Block_GetData(tmpBlock);
      memcpy(&isLeaf, data, sizeof(bool));  //And check if it is a leaf
    }else{  //Otherwise the loop has stopped because we reached to the right position of the block and now we go to the correct child block
      CALL_OR_DIE(BF_UnpinBlock(tmpBlock));

      CALL_OR_DIE(BF_GetBlock(openFiles[fd]->bf_desc, tmpBlockPtr, tmpBlock));
      data = BF_Block_GetData(tmpBlock);
      memcpy(&isLeaf, data, sizeof(bool));
    }
  }

  //After all this loops we are on the block that our key exists or it should at least
  data += sizeof(char); //So move to the block id the data pointer
  memcpy(&targetBlockId, data, sizeof(int));  //Get the block id
  free(tmpKey);
  CALL_OR_DIE(BF_UnpinBlock(tmpBlock));
  BF_Block_Destroy(&tmpBlock);

  return targetBlockId; //And return it
}

//Returning the id of the most left leaf
int findMostLeftLeaf(int fd){
  BF_Block *tmpBlock;
  BF_Block_Init(&tmpBlock);

  int rootId, tmpBlockPtr, targetBlockId;
  void *data;
  bool isLeaf = 0;

  rootId = openFiles[fd]->root_id;

  CALL_OR_DIE(BF_GetBlock(openFiles[fd]->bf_desc, rootId, tmpBlock)); //Get the root block to start searching
  data = BF_Block_GetData(tmpBlock);

  memcpy(&isLeaf, data, sizeof(bool));

  if (isLeaf == 1) //If the root is a leaf we are on the only leaf so the key should be here
  {
    CALL_OR_DIE(BF_UnpinBlock(tmpBlock));
    BF_Block_Destroy(&tmpBlock);
    return rootId;
  }

  while( isLeaf == 0){  //Everytime we get in a new block to search that is not a leaf

    data += (sizeof(char) + sizeof(int)*3);  //Move the data pointer over the isLeaf byte,the block id, the next block pointer and the records num they are useless for now

    memcpy(&tmpBlockPtr, data, sizeof(int));  // Get the first pointer to child block that exist in this block

    CALL_OR_DIE(BF_UnpinBlock(tmpBlock));

    CALL_OR_DIE(BF_GetBlock(openFiles[fd]->bf_desc, tmpBlockPtr, tmpBlock));  //Get to the child block
    data = BF_Block_GetData(tmpBlock);
    memcpy(&isLeaf, data, sizeof(bool));
  }

  //After all this loops we are on the leaf block that is the most left
  data += sizeof(char); //So move to the block id the data pointer
  memcpy(&targetBlockId, data, sizeof(int));  //Get the block id

  CALL_OR_DIE(BF_UnpinBlock(tmpBlock));
  BF_Block_Destroy(&tmpBlock);

  return targetBlockId; //And return it
}

//findRecord finds the position [0-n) of the record that has attr1 as value1 if it exists, otherwise the position it would be
int findRecordPos(void * data, int fd, void * value1){
  int records_no;
  int len1 = openFiles[fd]->length1;
  int len2 = openFiles[fd]->length2;
  int type = openFiles[fd]->type1;

  //get the number of records
  memmove(&records_no, data+sizeof(bool)+2*sizeof(int), sizeof(int));
  //add the leaf metadata to offset
  int offset = sizeof(bool) + 3*sizeof(int);
  int curr_record = 0;
  //while value1 is GREATER_THAN the current record (find the first record that is GREATER_THAN_OR_EQUAL to value1)
  while(keysComparer(value1, data+offset, GREATER_THAN, type) && curr_record < records_no){
    curr_record++;
    offset += len1+len2;
  }
  //if the correct position was not found then it is the next one
  return curr_record;
}

//RecordIndex->the index the new record should go [0-n), fd our openFiles descriptor, currRecords the amount of existing records in this block
//value1,2 the values to be inserted in the block
void simpleInsertToLeaf(int recordIndex, int fd, void* data, int currRecords, void *value1, void *value2){
  int offset, len1, len2;
  len1 = openFiles[fd]->length1;
  len2 = openFiles[fd]->length2;

  offset = (sizeof(char) + sizeof(int)*3 + recordIndex*(len1 + len2));  //Get the offset to that position
  //Move all the records after that position right by recordSize (len1 + len 2)
  memmove(data + offset + len1 + len2, data + offset, (currRecords - recordIndex)*(len1 + len2));
  memcpy(data + offset, value1, len1);  //Write to the space we made the new record
  offset += len1;
  memcpy(data + offset, value2, len2);
  currRecords++;
  offset = (sizeof(char) + sizeof(int)*2);
  memcpy(data + offset, &currRecords, sizeof(int)); //Write the new current number of records to the block
}

/************************************************
**************Create*******************************
*************************************************/
//Checks if the attributes are correct and if they agreee with the length given for them
int typeChecker(char attrType, int attrLength, int *type, int *len){
  if (attrType == INTEGER)
  {
    *type = 1; //If type is equal to 1 the attribute is int
  }else if (attrType == FLOAT)
  {
    *type = 2; //If type is equal to 2 the attribute is float
  }else if (attrType == STRING)
  {
    *type = 3; //If type is equal to 3 the attribute is string
  }else{
    return AME_WRONGARGS;
  }

  *len = attrLength;

  if (*type == 1 || *type == 2) // Checking if the argument type given matches the argument size given
  {
    if (*len != 4)
    {
      return AME_WRONGARGS;
    }
  }else{
    if (*len < 1 || *len > 255)
    {
      return AME_WRONGARGS;
    }
  }

  return AME_OK;
}

//Initializing a new blocks metadata
void blockMetadataInit(void *data, bool isLeaf, int blockId, int nextPtr, int recordsNum){
  //bool isLeaf
  memcpy(data, &isLeaf, sizeof(char));
  //int blockId
  data += sizeof(bool);
  memcpy(data, &blockId, sizeof(int)); //Writing the block's id to it
  //int nextPtr
  data += sizeof(int);
  memcpy(data, &nextPtr, sizeof(int));
  //int recordsNum
  data += sizeof(int);
  memcpy(data, &recordsNum, sizeof(int));
}

//Returning how many keys are equal to the target key
int sameKeysCount(void *data, void *targetKey, int length, int type, int currRecords){
  int sameKeysCounter = 0;
  int offset;
  void *tmpKey = NULL;
  int currIndex = 0;
  offset = sizeof(char) + sizeof(int)*3;
  memcpy(tmpKey, data + offset, length);
  while(currIndex < currRecords){
    if (keysComparer(targetKey, tmpKey, EQUAL, type))
    {
      sameKeysCounter++;
    }
    currIndex++;
  }
  return sameKeysCounter;
}

void print_metadata(char* data){
  char * di[7];
  int type1; //Type of the first(key) attribute. 1 for int 2 for float 3 for string
  int len1; //Length of the first attribute
  int type2; //Type of the second attribute. 1 for int 2 for float 3 for string
  int len2; //Length of the second attribute
  int rootID; //The block Id of the root
  int rootInitialized; //The root is not initialized yet, no record has been inserted
  int offset =0;

  memmove(&type1, data, sizeof(int));
  offset+=sizeof(int);
  memmove(&len1, data+offset, sizeof(int));
  offset+=sizeof(int);
  memmove(&type2, data+offset, sizeof(int));
  offset+=sizeof(int);
  memmove(&len2, data+offset, sizeof(int));
  offset+=sizeof(int);
  memmove(&rootID, data+offset, sizeof(int));
  offset+=sizeof(int);
  memmove(&rootInitialized, data+offset, sizeof(int));
  offset+=sizeof(int);
  printf("Printing first block\n");
  printf("Type1 %d\n", type1);
  printf("Len1 %d\n", len1);
  printf("Type2 %d\n", type2);
  printf("Len2 %d\n", len2);
  printf("rootID %d\n", rootID);

}

void print_leaf(char * data){
  int offset = 0;
  bool isleaf =0;
  int block_id = 0;
  int nextPtr=0;
  int recordsNum;
  memmove(&isleaf, data, sizeof(bool));
  offset+= sizeof(bool);
  memmove(&block_id, data+offset, sizeof(int));
  offset+= sizeof(int);
  memmove(&nextPtr, data+offset, sizeof(int));
  offset += sizeof(int);
  memmove(&recordsNum, data+offset, sizeof(int));
  printf("Printing leaf block with id %d\n", block_id);
  printf("Is leaf: %d\n", isleaf);
  printf("Nexptr %d\n", nextPtr);
  printf("recordsNum %d", recordsNum);
}

void PrintTree(int fileDesc){
  BF_Block *block;
  BF_Block_Init(&block);
  BF_GetBlock(openFiles[fileDesc]->bf_desc, 0, block);
  char* data = BF_Block_GetData(block);

  PrintBlockMetadata(data);

  if(!BlockIsLeaf(data)){
    PrintIndexBlock(data,fileDesc);
  }
  else{

  }
}

void PrintBlockMetadata(char* data){
  int isLeaf,blockId,nextPtr,recordsNum;
  memmove(&isLeaf,data,sizeof(bool));
  memmove(&blockId,data+sizeof(bool),sizeof(int));
  memmove(&nextPtr,data+sizeof(bool)+sizeof(int),sizeof(int));
  memmove(&recordsNum,data+sizeof(bool)+2*sizeof(int),sizeof(int));
  printf("%c.%d.%d.%d|||", isLeaf,blockId,nextPtr,recordsNum);
}

int BlockIsLeaf(char* data){
  int isLeaf;
  memmove(&isLeaf,data,sizeof(bool));
  return isLeaf;
}

void PrintLeafBlock(char* data, int fd){
  int num_of_records;
  memmove(&num_of_records,data+sizeof(bool)+2*sizeof(int),sizeof(int));

  int type1 = openFiles[fd]->type1;
  int type2 = openFiles[fd]->type2;
  int len1 = openFiles[fd]->length1;
  int len2 = openFiles[fd]->length2;

  int offset = sizeof(bool)+3*sizeof(int);
  for(int i=0; i<num_of_records; i++){
    PrintAttr(data+offset,type1,len1);
    printf("-");
    PrintAttr(data+offset+len1,type2,len2);
    printf("-");
    offset += len1+len2;
  }
}

void PrintIndexBlock(char *data, int fileDesc){/*
  int currKeys = 0;
  int i;
  int offset = sizeof(char) + sizeof(int)*2;
  int blockPtr, type, len;
  blockPtr = 0;
  len = openFiles[fileDesc]->length1;
  type = openFiles[fileDesc]->type1;
  char *key;
  key = malloc(openFiles[fileDesc]->length1);

  memcpy(&currKeys, data + offset, sizeof(int));
  offset += sizeof(int);

  for (i = 0; i < currKeys; ++i)
  {
    memcpy(&blockPtr, data + offset, sizeof(int));
    offset+=sizeof(int);
    memcpy(key, data + offset, len);
    offset+=len;
    if (type == INTEGER)
    {
      printf("%d-%d-", blockPtr, key);
    }else if (type == FLOAT)
    {
      printf("%d-%f-", blockPtr, key);
    }else{
      printf("%d-%s-", blockPtr, key);
    }
  }
  memcpy(&blockPtr, data + offset, sizeof(int));
  printf("%d\n", tmpBlockPtr);

  free(key);
*/}


void PrintAttr(char* data, int type, int len){
  int i;
  float f;
  char* str = malloc(len);
  switch(type){
    case 1: //int
      memmove(&i,data,sizeof(int));
      printf("%d", i);
      break;
    case 2: //float
      memmove(&f,data,sizeof(float));
      printf("%f", f);
      break;
    case 3: //string
      memmove(str,data,len);
      printf("%s", str);
  }
  free(str);
}
