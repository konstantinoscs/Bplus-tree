#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "AM.h"
#include "file_info.h"
#include "BoolType.h"
#include "Scan.h"
#include "stack.h"

#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      printf("Error\n");      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }                           \



typedef enum AM_ErrorCode {
  OK,
  OPEN_SCANS_FULL
}AM_ErrorCode;

//openFiles holds the info of open files in the appropriate index
file_info * openFiles[20];

/************************************************
**************Scan*******************************
*************************************************/


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
  memcpy(data, &isLeaf, sizeof(char));
  data += sizeof(char);

  memcpy(data, &blockId, sizeof(int)); //Writing the block's id to it
  data += sizeof(int);

  memcpy(data, &nextPtr, sizeof(int));
  data += sizeof(int);

  memcpy(data, &recordsNum, sizeof(int));
}

/************************************************
**************Insert*******************************
*************************************************/

//Takes the search key(target key) and the traversing key(tmp key), the operation (EQUAL, LESS_THAN etc) that we want to apply on the keys and the keytype
//and returns if the operation is true or false
//is targetkey op tmpkey?
bool keysComparer(void *targetKey, void *tmpKey, int operation, int keyType){
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
          if (!strcmp((char *)targetKey, (char *)tmpKey)) //When comparing strings we dont need to know their length
                                                          //because strcmp stops to the null term char
          {
            return 1;
          }
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
}

//Returning the stack full with the path to the leaf and the id of the leaf
//that has the key we are looking for or should have it at least. If called with null leafPath we didnt need
//to keep tha path in this function call
int findLeaf(int fd, void *key, Stack **leafPath){
  BF_Block *tmpBlock;
  BF_Block_Init(&tmpBlock);

  int keyType, keyLength, rootId, tmpBlockPtr, keysNumber, targetBlockId, tmpBlockId;
  void *data, *tmpKey;
  bool isLeaf = 0;

  //Get the type and the length of this file's key and the root
  keyType = openFiles[fd]->type1;
  keyLength = openFiles[fd]->length1;
  rootId = openFiles[fd]->root_id;

  CALL_OR_DIE(BF_GetBlock(openFiles[fd]->bf_desc, rootId, tmpBlock)); //Get the root block to start searching
  data = BF_Block_GetData(tmpBlock);

  memcpy(&isLeaf, data, sizeof(bool));

  /*if (isLeaf == 1) //If the root is a leaf we are on the only leaf so the key should be here
  {
    CALL_OR_DIE(BF_UnpinBlock(tmpBlock));
    BF_Block_Destroy(&tmpBlock);
    return rootId;
  }*/

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
      memcpy(tmpKey, data, sizeof(int)); //get the new tmp key
      data += sizeof(int);
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
  int offset = 0;
  int record = 0;
  int records_no;
  int len1 = openFiles[fd]->length1;
  int len2 = openFiles[fd]->length2;
  int type = openFiles[fd]->type1;

  //add the first static data to offset
  offset += sizeof(bool) + 2*sizeof(int);
  //get the number of records
  memmove(&records_no, data+offset, sizeof(int));
  //add to the offset 4 bytes for keys_no
  offset += sizeof(int);

  //iterate for each record
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

//RecordIndex->the index the new record should go [0-n), fd our openFiles descriptor, currRecords the amount of existing records in this block
//value1,2 tha values to be inserted in the block
void simpleInsertToLeaf(int recordIndex, int fd, void *data, int currRecords, void *value1, void *value2){
  int offset, len1, len2;
  len1 = openFiles[fd]->length1;
  len2 = openFiles[fd]->length2;

  offset = (sizeof(char) + sizeof(int)*3 + recordIndex*(len1 + len2));  //Get the offset to that position
  memmove(data + offset + len1 + len2, data + offset, (currRecords - recordIndex)*(len1 + len2)); //Move all the records after that position right by recordSize (len1 + len 2)
  memcpy(data + offset, value1, len1);  //Write to the space we made the new record
  offset += len1;
  memcpy(data + offset, value2, len2);
  currRecords++;
  offset = (sizeof(char) + sizeof(int)*2);
  memcpy(data + offset, &currRecords, sizeof(int)); //Write the new current number of records to the block
}

/************************************************
**************FindNextEntry*******************************
*************************************************/

//findRecord finds the next record which attr1 has value1 and returns
//its offset from the start of the block
int findRecord(void * data, int fd, void * value1){
  int offset = 0;
  int record = 0;
  int records_no;
  int len1 = openFiles[fd]->length1;
  int len2 = openFiles[fd]->length2;
  int type = openFiles[fd]->type1;

  //add the first static data to offset
  offset += sizeof(bool) + 2*sizeof(int);
  //get the number of records
  memmove(&records_no, data+offset, sizeof(int));
  //add to the offset 4 bytes for keys_no
  offset += sizeof(int);

  while(!keysComparer(value1, data+offset, EQUAL, type)){
    offset += len1 + len2;
    //if went through all the blocks return -1
    if(++record == records_no)
      return -1;
  }
  return record-1;
}

/***************************************************
***************AM_Epipedo***************************
****************************************************/

int AM_errno = AME_OK;

void AM_Init() {
  BF_Init(MRU);
	return;
}


int AM_CreateIndex(char *fileName, char attrType1, int attrLength1, char attrType2, int attrLength2) {

  int type1,type2,len1,len2;

  if (typeChecker(attrType1, attrLength1, &type1, &len1) != AME_OK)
  {
    return AME_WRONGARGS;
  }

  if (typeChecker(attrType2, attrLength2, &type2, &len2) != AME_OK)
  {
    return AME_WRONGARGS;
  }

  /*attrMeta.type1 = type1;
  attrMeta.len1 = len1;
  attrMeta.type2 = type2;
  attrMeta.len2 = len2;*/

  int fd;
  //temporarily insert the file in openFiles
  int file_index = insert_file(type1, len1, type2, len2);
  if(file_index == -1)
    return AME_MAXFILES;

  BF_Block *tmpBlock;
  BF_Block_Init(&tmpBlock);

  CALL_OR_DIE(BF_CreateFile(fileName));
  CALL_OR_DIE(BF_OpenFile(fileName, &fd));

  //put the bf level decriptor after you opened
  insert_bfd(file_index, fd);

  void *data;
  char keyWord[15];
  //BF_AllocateBlock(fd, tmpBlock);
  CALL_OR_DIE(BF_AllocateBlock(fd, tmpBlock));//Allocating the first block that will host the metadaτa

  data = BF_Block_GetData(tmpBlock);
  strcpy(keyWord,"DIBLU$");
  memcpy(data, keyWord, sizeof(char)*15);//Copying the key-phrase DIBLU$ that shows us that this is a B+ file
  data += sizeof(char)*15;
  //Writing the attr1 and attr2 type and length right after the keyWord in the metadata block
  memcpy(data, &type1, sizeof(int));
  data += sizeof(int);
  memcpy(data, &len1, sizeof(int));
  data += sizeof(int);
  memcpy(data, &type2, sizeof(int));
  data += sizeof(int);
  memcpy(data, &len2, sizeof(int));

  BF_Block_SetDirty(tmpBlock);
  CALL_OR_DIE(BF_UnpinBlock(tmpBlock));

  //Allocating the root block
  CALL_OR_DIE(BF_AllocateBlock(fd, tmpBlock));
  int blockNum, nextPtr, recordsNum;
  CALL_OR_DIE(BF_GetBlockCounter(fd, &blockNum));
  blockNum--;
  insert_root(file_index, blockNum);

  data = BF_Block_GetData(tmpBlock);

  //Inserting data to the root, arguments: 0 means not a leaf, blockNum is its id, -2 means no nextPtr since its not leaf, 0 records yet inserted
  blockMetadataInit(data, 0, blockNum, -2, 0);

  BF_Block_SetDirty(tmpBlock);
  CALL_OR_DIE(BF_UnpinBlock(tmpBlock));


  //Allocating the down right empty leaf block
  int downRightBlock;
  CALL_OR_DIE(BF_AllocateBlock(fd, tmpBlock));
  CALL_OR_DIE(BF_GetBlockCounter(fd, &downRightBlock));
  downRightBlock--;

  data = BF_Block_GetData(tmpBlock);

  //Inserting data to the new block, arguments: 1 means a leaf, downRightBlock is its id, -1 means its the down right leaf, 0 records yet inserted
  blockMetadataInit(data, 1, downRightBlock, -1, 0);

  BF_Block_SetDirty(tmpBlock);
  CALL_OR_DIE(BF_UnpinBlock(tmpBlock));  


  //Allocating the down left empty leaf block
  int downLeftBlock;
  CALL_OR_DIE(BF_AllocateBlock(fd, tmpBlock));
  CALL_OR_DIE(BF_GetBlockCounter(fd, &downLeftBlock));
  downLeftBlock--;

  data = BF_Block_GetData(tmpBlock);

  //Inserting data to the new block, arguments: 1 means a leaf, downLeftBlock is its id, downRightBlock means its next the down right leaf, 0 records yet inserted
  blockMetadataInit(data, 1, downLeftBlock, downRightBlock, 0);

  BF_Block_SetDirty(tmpBlock);
  CALL_OR_DIE(BF_UnpinBlock(tmpBlock));


  //Getting the root block to inform it about its new childs
  CALL_OR_DIE(BF_GetBlock(fd, blockNum, tmpBlock));
  data = BF_Block_GetData(tmpBlock);
  data += (sizeof(char) + sizeof(int)*3);
  memcpy(data, &downLeftBlock, sizeof(int));
  data += (sizeof(int) + len1);
  memcpy(data, &downRightBlock, sizeof(int));


  //Getting again the first (the metadata) block to write after the attributes info the root block id
  CALL_OR_DIE(BF_GetBlock(fd, 0, tmpBlock));
  data = BF_Block_GetData(tmpBlock);
  data += (sizeof(char)*15 + sizeof(int)*4);
  //printf("GRAFW %d\n", blockNum);
  memcpy(data, &blockNum, sizeof(int));
  BF_Block_SetDirty(tmpBlock);
  CALL_OR_DIE(BF_UnpinBlock(tmpBlock));

  BF_Block_Destroy(&tmpBlock);
  CALL_OR_DIE(BF_CloseFile(fd));
  //remove the file from openFiles array
  close_file(file_index);

  return AME_OK;
}


int AM_DestroyIndex(char *fileName) {
  return AME_OK;
}


int AM_OpenIndex (char *fileName) {
  BF_Block *tmpBlock;
  BF_Block_Init(&tmpBlock);

  int type1,type2,len1,len2, fileDesc, rootId;
  char *data = NULL;


  CALL_OR_DIE(BF_OpenFile(fileName, &fileDesc));

  CALL_OR_DIE(BF_GetBlock(fileDesc, 0, tmpBlock));//Getting the first block
  data = BF_Block_GetData(tmpBlock);//and its data

  if (data == NULL || strcmp(data, "DIBLU$"))//to check if this new opened file is a B+ tree file
  {
    printf("File: %s is not a B+ tree file. Exiting..\n", fileName);
    exit(-1);
  }

  data += sizeof(char)*15; //skip the diblus keyword
  //Getting the attr1 and attr2 type and length from the metadata block
  memcpy(&type1, data, sizeof(int));
  data += sizeof(int);
  memcpy(&len1, data, sizeof(int));
  data += sizeof(int);
  memcpy(&type2, data, sizeof(int));
  data += sizeof(int);
  memcpy(&len2, data, sizeof(int));
  data += sizeof(int);
  memcpy(&rootId, data, sizeof(int));

  CALL_OR_DIE(BF_UnpinBlock(tmpBlock));
  BF_Block_Destroy(&tmpBlock);


  int file_index = insert_file(type1, len1, type2, len2);
  //check if we have reached the maximum number of files
  if(file_index == -1)
    return AME_MAXFILES;

  insert_bfd(file_index, fileDesc);
  insert_root(file_index, rootId);

  /*data += sizeof(char)*15;
  int type, len;
  memcpy(&type, data, sizeof(int));
  data += sizeof(int);
  memcpy(&len, data, sizeof(int));

  printf("%d %d\n", type, len);*/

  //return the file index
  return file_index;
}


int AM_CloseIndex (int fileDesc) {
  //TODO other stuff?
  CALL_OR_DIE(BF_CloseFile(openFiles[fileDesc]->bf_desc));
  //remove the file from the openFiles array
  close_file(fileDesc);
  return AME_OK;
}


int AM_InsertEntry(int fileDesc, void *value1, void *value2) {

/*  void *data1 = NULL;
  void *data2 = NULL;

  int type1, len1, type2, len2, targetBlockId, recordIndex, nextPtr;
  int offset = 0;
  int currRecords, maxRecords;
  Stack *nodesPath;
  create_stack(&nodesPath);

  //Getting the attr1 and attr2 type and length
  type1 = openFiles[fileDesc]->type1;
  len1 = openFiles[fileDesc]->length1;

  type2 = openFiles[fileDesc]->type2;
  len2 = openFiles[fileDesc]->length2;

  targetBlockId = findLeaf(openFiles[fileDesc]->bf_desc, value1, &nodesPath); //Find the leaf that this value is supposed to be inserted and the path getting there

  BF_Block *tmpBlock;
  BF_Block_Init(&tmpBlock);
  CALL_OR_DIE(BF_GetBlock(openFiles[fileDesc]->bf_desc, targetBlockId, tmpBlock));//Getting the block that we are supposed to insert the new record
  data1 = BF_Block_GetData(tmpBlock);//and its data

  offset = (sizeof(char) + sizeof(int));

  memcpy(&nextPtr, data1 + offset, sizeof(int));// Getting the nextPtr of the targed block
  offset += sizeof(int);
  memcpy(&currRecords, data1 + offset, sizeof(int)); //Getting the amount of records that exist in this block

  maxRecords = (BF_BLOCK_SIZE - (sizeof(char) + sizeof(int)*3))/(len1 + len2);

  recordIndex = findRecordPos(data1, fileDesc, value1);  //Get the position it should go

  if (currRecords < maxRecords)
  {
    simpleInsertToLeaf(recordIndex, fileDesc, data1, currRecords, value1, value2);
  }else{   //If this block is full we are going to split it and divide its data
    //Allocating and initializing the new block
    CALL_OR_DIE(BF_AllocateBlock(openFiles[fileDesc]->bf_desc, tmpBlock));
    int blockId;
    CALL_OR_DIE(BF_GetBlockCounter(openFiles[fileDesc]->bf_desc, &blockId));
    blockId--;

    offset = (sizeof(char) + sizeof(int));
    memcpy(data1, &blockId, sizeof(int));  //Set as next of the old block the new block

    data2 = BF_Block_GetData(tmpBlock);

    //Inserting data to the new block, arguments: 1 means a leaf, blockId is its id,
    //nextPtr means the next block of the new block is the one that was next to the old block, 0 records yet inserted
    blockMetadataInit(data2, 1, blockId, nextPtr, 0); //STEFANIDI EDW EISAI

    bool goesToOld = 0; //If 0 the new record will go to the new block else it goes to the old block
    if (recordIndex + 1 <= maxRecords/2)
    {
      goesToOld = 1;
    }

    int numRecToOld = maxRecords/2;  //How many records will stay to the old block
    if (!(maxRecords%2))  //If the max records number is odd
    {
      if (!goesToOld) //If the new record belongs to the new block
      {
        numRecToOld++;  //Then increase by one the number of records that will go to the old one to keep the balance
      }
    }else{  //If the max records number is even
      if (goesToOld)  //If the new record belongs to the old block
      {
        numRecToOld--;  //Then decrease by one the number of records that will go to the old one to keep the balance
      }
    }

    offset = (sizeof(char) + sizeof(int)*3);
    int offset2 = sizeof(int);
    int numRecToNew = maxRecords - numRecToOld; //Calculate how many records will the new block have

    //Move to the start of the new block the data that is pointed by data1 (old block) increased by the number of records that
    //should stay in the old block. The segment that is to be moved will have the size of the records that should be moved on the new block
    memcpy(data2 + offset2, data1 + offset + numRecToOld*(len1 + len2), numRecToNew*(len1 + len2));

    offset = sizeof(char) + sizeof(int)*2;
    //Updating the block counters of the old and the new block
    memcpy(data1 + offset, &numRecToOld, sizeof(int));
    memcpy(data2, &numRecToNew, sizeof(int));

    void *newKey = NULL;

    data2 += sizeof(int);
    memcpy(newKey, data2, len1); //Take the first attribute of the new block to take its key STEFANIDI EDW EISAI
    data2 = BF_Block_GetData(tmpBlock);

    if (goesToOld)  //If the new record is to go to the old block
    {
      recordIndex = findRecordPos(data1, fileDesc, value1);  //Get the position it should go again in case something changed
      simpleInsertToLeaf(recordIndex, fileDesc, data1, numRecToOld, value1, value2); //Make a simple insertion to the old block
    }else{//If the new record is to go to the new block
      recordIndex = findRecordPos(data2, fileDesc, value1);  //Get the position it should go again in case something changed
      simpleInsertToLeaf(recordIndex, fileDesc, data2, numRecToNew, value1, value2);  //Make a simple insertion to the old block
    }

    destroy_stack(nodesPath);



      

    //NA KANW DIRTY KAI UNPIN TA BLOCK

    //An einai gemato tote prepei na to spasoume se 2 nea blok kai na mirasoume tis times se afta. me kapion tropo na pame ston apo panw komvo tou kai
    //na tsekaroume ean einai gematos kai aftos. An oxi vazoume to neo klidi ston epanw kai ola kala an einai gematos pali ton spame kai kanoume tin proigoumeni
    //diadikasia apo tin arxi.

    //While to block einai full
    //pare to key pou thes na kaneis insert
    //vres prin apo pio record prepei na mpei (se pia thesi)
    //kane allocate neo mplok
    //metafere ta misa records sto neo mplok ipologizontas oti to neo exei idi mpei kai meta kanto apli insert se ena apta dio mplok (sto swsto)
    //(ean i thesi pou tha empene einai megaliteri tou max records number / 2 tote paei sto 2o)
    //ftiakse tous metrites record sto neo kai sto palio mplok swstous
    //pare to prwto record tou neou mplok
    //popare apo tin stack
    //pigene sto mplok pou popares
    //prospathise na prostheiseis to prwto record tou neou mplok
  }


  CALL_OR_DIE(BF_UnpinBlock(tmpBlock));
  BF_Block_Destroy(&tmpBlock);*/

  return AME_OK;
}


int AM_OpenIndexScan(int fileDesc, int op, void *value) {
  Scan* scan = malloc(sizeof(Scan));
  scan->fileDesc = fileDesc;
	scan->op = op;
	scan->value = value;
	scan->block_num = -1;
	scan->record_num = -1;
  scan->ScanIsOver = false;
  scan->return_value = NULL;

	return openScansInsert(scan);
  //return AME_OK;
}


void *AM_FindNextEntry(int scanDesc) {
  Scan* scan = openScans[scanDesc];
  file_info* file = openFiles[scan->fileDesc];

  if(scan->ScanIsOver){
    if(scan->return_value != NULL)
      free(scan->return_value);
    return NULL;
  }

  switch(scan->op){
    case EQUAL:
                if(scan->return_value == NULL){  //first time this Scan is called
                  scan->return_value = malloc(sizeof(file->length2));
                  //find the leaf block this key belongs to
                  scan->block_num = findLeaf(scan->fileDesc,scan->value, NULL);
                  BF_Block* block;
                  BF_Block_Init(&block);
                  BF_GetBlock(file->bf_desc,scan->block_num,block);
                  //get its data
                  char* data = BF_Block_GetData(block);
                  //find the first record with scan->value in this block
                  scan->record_num = 0;
                  void* recordAttr1 = data+sizeof(char)+3*sizeof(int);
                  while(!keysComparer(recordAttr1,scan->value,EQUAL,file->type1)){
                    int next_record_offset = ScanNextRecord(scan,&block,&data);
                    if(next_record_offset == NO_NEXT_BLOCK)
                      return NULL;  //end the Scan
                    else
                      recordAttr1 = data+next_record_offset;
                  }
                  //now we've got a record that is EQUAL, so lets return it
                  memcpy(scan->return_value,recordAttr1+file->length1,sizeof(file->length2));
                  //clear block
                  BF_UnpinBlock(block);
                  BF_Block_Destroy(&block);
                  return scan->return_value;
                }
                else{                       //if this scan gets called again, we ned to check for more equal values (there arent any equals in different blocks)
                  BF_Block* block;
                  BF_Block_Init(&block);
                  BF_GetBlock(file->bf_desc,scan->block_num,block);
                  //get blocks data
                  char* data = BF_Block_GetData(block);
                  //look at the next record
                  int next_record_offset = ScanNextRecord(scan,&block,&data);
                  void* recordAttr1;
                  if(next_record_offset == NO_NEXT_BLOCK)
                    return NULL;  //end the Scan
                  else
                    recordAttr1 = data+next_record_offset;
                  //is the next record is equal as well?
                  if(keysComparer(recordAttr1, scan->value, EQUAL, file->type1)){  //if it is equal
                    memcpy(scan->return_value,recordAttr1+file->length1,sizeof(file->length2));
                    //clear block
                    BF_UnpinBlock(block);
                    BF_Block_Destroy(&block);
                    return scan->return_value;
                  }
                  else{                                         //if its not equal then end the scan (the rest wont be equal either)
                    free(scan->return_value);
                    scan->return_value = NULL;
                    scan->ScanIsOver = true;
                    //clear block
                    BF_UnpinBlock(block);
                    BF_Block_Destroy(&block);
                    return NULL;
                  }
                }
                break;
    case NOT_EQUAL:
                if(scan->return_value == NULL){  //first time this Scan is called
                  scan->return_value = malloc(sizeof(file->length2));
                  //find the left most block
                  scan->block_num = findMostLeftLeaf(scan->fileDesc);
                  BF_Block* block;
                  BF_Block_Init(&block);
                  BF_GetBlock(file->bf_desc,scan->block_num,block);
                  //get blocks data
                  char* data = BF_Block_GetData(block);
                  //find the first record of this block
                  scan->record_num = 0;
                  void* recordAttr1 = data+sizeof(char)+3*sizeof(int);
                  //is this record not_equal?
                  while(!keysComparer(recordAttr1,scan->value,NOT_EQUAL,file->type1)){  //if not check the next record until you find one that is not_equal
                    int next_record_offset = ScanNextRecord(scan,&block,&data);
                    if(next_record_offset == NO_NEXT_BLOCK)
                      return NULL;  //end the Scan
                    else
                      recordAttr1 = data+next_record_offset;
                  }
                  //now we've got a record that is not_equal, so lets return it
                  memcpy(scan->return_value,recordAttr1+file->length1,sizeof(file->length2));
                  //clear block
                  BF_UnpinBlock(block);
                  BF_Block_Destroy(&block);
                  return scan->return_value;
                }
                else{         //not the first time
                  BF_Block* block;
                  BF_Block_Init(&block);
                  BF_GetBlock(file->bf_desc,scan->block_num,block);
                  char* data = BF_Block_GetData(block);
                  void* num_of_records = data+sizeof(char)+2*sizeof(int);
                  void* recordAttr1;
                  //look at the next record
                  do{
                    int next_record_offset = ScanNextRecord(scan,&block,&data);
                    if(next_record_offset == NO_NEXT_BLOCK)
                      return NULL;  //if there is no next block we end the Scan
                    else
                      recordAttr1 = data+next_record_offset;
                  }while(!keysComparer(recordAttr1,scan->value,NOT_EQUAL,file->type1));  //is this record not_equal? if not check the next record untill you find one that is not_equal
                  //now we've got a record that is not_equal, so lets return it
                  memcpy(scan->return_value,recordAttr1+file->length1,sizeof(file->length2));
                  //clear block
                  BF_UnpinBlock(block);
                  BF_Block_Destroy(&block);
                  return scan->return_value;
                }
    case LESS_THAN:
                if(scan->return_value == NULL){ //fist time this scan is called
                  scan->return_value = malloc(sizeof(file->length2));
                  //find the left most block
                  scan->block_num = findMostLeftLeaf(scan->fileDesc);
                  BF_Block* block;
                  BF_Block_Init(&block);
                  BF_GetBlock(file->bf_desc,scan->block_num,block);
                  //get blocks data
                  char* data = BF_Block_GetData(block);
                  //find the first record of this block
                  scan->record_num = 0;
                  void* recordAttr1 = data+sizeof(char)+3*sizeof(int);
                  //is this record less_than?
                  while(!keysComparer(recordAttr1,scan->value,LESS_THAN,file->type1)){  //if not check the next record until you find one that is less_than
                    int next_record_offset = ScanNextRecord(scan,&block,&data);
                    if(next_record_offset == NO_NEXT_BLOCK)
                      return NULL;  //end the Scan
                    else
                      recordAttr1 = data+next_record_offset;
                  }
                  //now we've got a record that is less_than, so lets return it
                  memcpy(scan->return_value,recordAttr1+file->length1,sizeof(file->length2));
                  //clear block
                  BF_UnpinBlock(block);
                  BF_Block_Destroy(&block);
                  return scan->return_value;
                }
                else{   //if its not the first tiem
                  BF_Block* block;
                  BF_Block_Init(&block);
                  BF_GetBlock(file->bf_desc,scan->block_num,block);
                  char* data = BF_Block_GetData(block);
                  void* recordAttr1;
                  //look at the next record
                  do{
                    int next_record_offset = ScanNextRecord(scan,&block,&data);
                    if(next_record_offset == NO_NEXT_BLOCK)
                      return NULL;  //if there is no next block we end the Scan
                    else
                      recordAttr1 = data+next_record_offset;
                  }while(!keysComparer(recordAttr1,scan->value,LESS_THAN,file->type1));  //is this record less_than? if not check the next record untill you find one that is less_than
                  //now we've got a record that is less_than, so lets return it
                  memcpy(scan->return_value,recordAttr1+file->length1,sizeof(file->length2));
                  //clear block
                  BF_UnpinBlock(block);
                  BF_Block_Destroy(&block);
                  return scan->return_value;
                }
    case GREATER_THAN:
                if(scan->return_value == NULL){ //first time this scan is called
                  scan->return_value = malloc(sizeof(file->length2));
                  //find the leaf block this key belongs to
                  scan->block_num = findLeaf(scan->fileDesc,scan->value, NULL);
                  scan->record_num = 0;
                  BF_Block* block;
                  BF_Block_Init(&block);
                  BF_GetBlock(file->bf_desc,scan->block_num,block);
                  //get its data
                  char* data = BF_Block_GetData(block);
                  //find the first record with attribute1 >scan->value in this block
                  void* recordAttr1 = data+sizeof(char)+3*sizeof(int);
                  while(!keysComparer(recordAttr1,scan->value,GREATER_THAN,file->type1)){
                    int next_record_offset = ScanNextRecord(scan,&block,&data);
                    if(next_record_offset == NO_NEXT_BLOCK)
                      return NULL;  //if there is no next block we end the Scan
                    else
                      recordAttr1 = data+next_record_offset;
                  };
                  //now we've got a GREATER_THAN record, so lets return it
                  memcpy(scan->return_value,recordAttr1+file->length1,sizeof(file->length2));
                  //clear block
                  BF_UnpinBlock(block);
                  BF_Block_Destroy(&block);
                  return scan->return_value;
                }
                else{       //not the first time
                  BF_Block* block;
                  BF_Block_Init(&block);
                  BF_GetBlock(file->bf_desc,scan->block_num,block);
                  char* data = BF_Block_GetData(block);
                  void* recordAttr1;
                  //look at the next record
                  int next_record_offset = ScanNextRecord(scan,&block,&data);
                  if(next_record_offset == NO_NEXT_BLOCK)
                    return NULL;  //if there is no next block we end the Scan
                  else
                    recordAttr1 = data+next_record_offset;
                  //next record is deffinetely GREATER_THAN, so lets return it
                  memcpy(scan->return_value,recordAttr1+file->length1,sizeof(file->length2));
                  //clear block
                  BF_UnpinBlock(block);
                  BF_Block_Destroy(&block);
                  return scan->return_value;
                }
    case LESS_THAN_OR_EQUAL:
                if(scan->return_value == NULL){ //fist time this scan is called
                  scan->return_value = malloc(sizeof(file->length2));
                  //find the left most block
                  scan->block_num = findMostLeftLeaf(scan->fileDesc);
                  BF_Block* block;
                  BF_Block_Init(&block);
                  BF_GetBlock(file->bf_desc,scan->block_num,block);
                  //get blocks data
                  char* data = BF_Block_GetData(block);
                  //find the first record of this block
                  scan->record_num = 0;
                  void* recordAttr1 = data+sizeof(char)+3*sizeof(int);
                  scan->record_num = 0;
                  //is this record LESS_THAN_OR_EQUAL?
                  while(!keysComparer(recordAttr1,scan->value,LESS_THAN_OR_EQUAL,file->type1)){  //if not check the next record until you find one that is LESS_THAN_OR_EQUAL
                    int next_record_offset = ScanNextRecord(scan,&block,&data);
                    if(next_record_offset == NO_NEXT_BLOCK)
                      return NULL;  //end the Scan
                    else
                      recordAttr1 = data+next_record_offset;
                  }
                  //now we've got a record that is LESS_THAN_OR_EQUAL, so lets return it
                  memcpy(scan->return_value,recordAttr1+file->length1,sizeof(file->length2));
                  //clear block
                  BF_UnpinBlock(block);
                  BF_Block_Destroy(&block);
                  return scan->return_value;
                }
                else{   //if its not the first time
                  BF_Block* block;
                  BF_Block_Init(&block);
                  BF_GetBlock(file->bf_desc,scan->block_num,block);
                  char* data = BF_Block_GetData(block);
                  void* recordAttr1;
                  //look at the next record
                  do{
                    int next_record_offset = ScanNextRecord(scan,&block,&data);
                    if(next_record_offset == NO_NEXT_BLOCK)
                      return NULL;  //if there is no next block we end the Scan
                    else
                      recordAttr1 = data+next_record_offset;
                  }while(!keysComparer(recordAttr1,scan->value,LESS_THAN_OR_EQUAL,file->type1));  //is this record LESS_THAN_OR_EQUAL? if not check the next record untill you find one that
                  //now we've got a record that is LESS_THAN_OR_EQUAL, so lets return it
                  memcpy(scan->return_value,recordAttr1+file->length1,sizeof(file->length2));
                  //clear block
                  BF_UnpinBlock(block);
                  BF_Block_Destroy(&block);
                  return scan->return_value;
                }
    case GREATER_THAN_OR_EQUAL:
                if(scan->return_value == NULL){ //first time this scan is called
                  scan->return_value = malloc(sizeof(file->length2));
                  //find the leaf block this key belongs to
                  scan->block_num = findLeaf(scan->fileDesc,scan->value, NULL);
                  scan->record_num = 0;
                  BF_Block* block;
                  BF_Block_Init(&block);
                  BF_GetBlock(file->bf_desc,scan->block_num,block);
                  //get its data
                  char* data = BF_Block_GetData(block);
                  //find the first record with attribute1 >=scan->value in this block
                  void* recordAttr1 = data+sizeof(char)+3*sizeof(int);
                  while(!keysComparer(recordAttr1,scan->return_value,GREATER_THAN_OR_EQUAL,file->type1)){
                    int next_record_offset = ScanNextRecord(scan,&block,&data);
                    if(next_record_offset == NO_NEXT_BLOCK)
                      return NULL;  //end the Scan
                    else
                      recordAttr1 = data+next_record_offset;
                  }
                  //now we've got a record that is GREATER_THAN_OR_EQUAL, so lets return it
                  memcpy(scan->return_value,recordAttr1+file->length1,sizeof(file->length2));
                  //clear block
                  BF_UnpinBlock(block);
                  BF_Block_Destroy(&block);
                  return scan->return_value;
                }
                else{       //not the first time
                  BF_Block* block;
                  BF_Block_Init(&block);
                  BF_GetBlock(file->bf_desc,scan->block_num,block);
                  char* data = BF_Block_GetData(block);
                  void* recordAttr1;
                  //look at the next record
                  int next_record_offset = ScanNextRecord(scan,&block,&data);
                  if(next_record_offset == NO_NEXT_BLOCK)
                    return NULL;  //if there is no next block we end the Scan
                  else
                    recordAttr1 = data+next_record_offset;
                  //next record is deffinetely GREATER_THAN, so lets return it
                  memcpy(scan->return_value,recordAttr1+file->length1,sizeof(file->length2));
                  //clear block
                  BF_UnpinBlock(block);
                  BF_Block_Destroy(&block);
                  return scan->return_value;
                }
  }
}

int AM_CloseIndexScan(int scanDesc) {
  Scan* scan = openScans[scanDesc];
  if(scan->return_value != NULL)
    free(scan->return_value);
  free(scan);
  openScans[scanDesc] = NULL;
  return AME_OK;
}


void AM_PrintError(char *errString) {
  printf("%s\n", errString);
}

void AM_Close() {
  BF_Close();
  delete_files();
}
