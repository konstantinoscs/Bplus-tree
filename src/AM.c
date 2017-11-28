#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "AM.h"
#include "file_info.h"

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

//openFiles holds the names of open files in the appropriate index
file_info * openFiles[20];

/************************************************
**************Scan*******************************
*************************************************/

typedef struct Scan {
	int fileDesc;			//the file that te scan refers to
	int block_num;		//last block that was checked
	int record_num;		//last record that was checked
	int op;			//the operation
	void *value;			//the target value
  bool ScanIsOver;
}Scan;

#define MAX_SCANS 20
Scan* openScans[MAX_SCANS];  //This is where open Scans are saved. The array is initialized with NULL's.

int openScansInsert(Scan* scan);  //inserts a Scan in openScans[] if possible and returns the position
int openScansFindEmptySlot();     //finds the first empty slot in openScans[]
bool openScansFull();             //checks


int openScansInsert(Scan* scan){
	int pos = openScansFindEmptySlot();
  if(openScansFull() != true)
    openScans[pos] = scan;
  else{
    fprintf(stderr, "openScans[] can't fit more Scans! Exiting...\n");
    exit(0);
  }
  return pos;
}

int openScansFindEmptySlot(){
	int i;
	for(i=0; i<MAX_SCANS; i++)
		if(openScans[i] == NULL)
			return i;
	return i;
}

bool openScansFull(){
	if(openScansFindEmptySlot() == MAX_SCANS) //if you cant find empty slot in [0-19] then its full
		return true;
	return false;
}

/************************************************
**************Create*******************************
*************************************************/

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

/************************************************
**************Insert*******************************
*************************************************/

int findLeaf(int fd, int key){
  BF_Block *tmpBlock;
  BF_Block_Init(&tmpBlock);

  int keyType, keyLength, rootId, tmpBlockPtr, tmpKey, keysNumber, targetBlockId;
  void *data;
  char isLeaf = 0;

  //Getting the first block to have access in the first attrr abd the root block id
  CALL_OR_DIE(BF_GetBlock(openFiles[fd]->bf_desc, 0, tmpBlock));
  data = BF_Block_GetData(tmpBlock);

  data += sizeof(char)*15; //move further from the keyword

  //Get the type and the length of this file's key and its root Id
  memcpy(&keyType, data, sizeof(int));
  data += sizeof(int);
  memcpy(&keyLength, data, sizeof(int));
  data += (sizeof(int)*3);
  memcpy(&rootId, data,sizeof(int));

  CALL_OR_DIE(BF_UnpinBlock(tmpBlock));

  CALL_OR_DIE(BF_GetBlock(openFiles[fd]->bf_desc, rootId, tmpBlock)); //Get the root block to start searching
  data = BF_Block_GetData(tmpBlock);

  memcpy(&isLeaf, data, sizeof(char));

  if (isLeaf == 1) //If the root is a leaf we are on the only leaf so the key should be here
  {
    CALL_OR_DIE(BF_UnpinBlock(tmpBlock));
    BF_Block_Destroy(&tmpBlock);
    return rootId;
  }

  while( isLeaf == 0){  //Everytime we get in a new block to search that is not a leaf

    int currKey = 0;  //Initialize the index of keys pointer

    data += (sizeof(char) + sizeof(int)*2);  //Move the data pointer over the isLeaf byte,the block id and the next block pointer they are useless for now

    memcpy(&keysNumber, data, sizeof(int)); //Get the number of the keys that exist in this block
    data += sizeof(int);
    memcpy(&tmpBlockPtr, data, sizeof(int));  // Get the first pointer to child block that exist in this block
    data += sizeof(int);
    memcpy(&tmpKey, data, sizeof(int)); //Get the value of the first key in this block
    data += sizeof(int);
    currKey++;  //Increase the index pointer

    while(key >= tmpKey && currKey < keysNumber){ //while the key that we look for is bigger than the key that we have now
      //and the index number is smaller than the amount of keys in this block (so we are not on the last key) keep traversing
      memcpy(&tmpBlockPtr, data, sizeof(int));  //get the pointer to the child block that is before the new tmpKey
      data += sizeof(int);
      memcpy(&tmpKey, data, sizeof(int)); //get the new tmp key
      data += sizeof(int);
      currKey++;
    }

    if (key >= tmpKey)  //if the loop stopped because we reached the last key on this block but still the key
    //that we are looking for is bigger than the last key
    {
      memcpy(&tmpBlockPtr, data, sizeof(int));  //Then get the last child pointer of this block
      CALL_OR_DIE(BF_UnpinBlock(tmpBlock));

      CALL_OR_DIE(BF_GetBlock(openFiles[fd]->bf_desc, tmpBlockPtr, tmpBlock));  //Get to this block
      data = BF_Block_GetData(tmpBlock);
      memcpy(&isLeaf, data, sizeof(char));  //And check if it is a leaf
    }else{  //Otherwise the loop has stopped because we reached to the right position of the block and now we go to the correct child block
      CALL_OR_DIE(BF_UnpinBlock(tmpBlock));

      CALL_OR_DIE(BF_GetBlock(openFiles[fd]->bf_desc, tmpBlockPtr, tmpBlock));
      data = BF_Block_GetData(tmpBlock);
      memcpy(&isLeaf, data, sizeof(char));
    }
  }

  //After all this loops we are on the block that our key exists or it should at least
  data += sizeof(char); //So move to the block id the data pointer
  memcpy(&targetBlockId, data, sizeof(int));  //Get the block id

  CALL_OR_DIE(BF_UnpinBlock(tmpBlock));
  BF_Block_Destroy(&tmpBlock);

  return targetBlockId; //And return it
}

/***************************************************
***************AM_Epipedo***************************
****************************************************/

int AM_errno = AME_OK;

void AM_Init() {
  for(int i=0; i<20; i++){
    if(openFiles[i] == NULL){
      printf("%d is null \n", i);
    }
  }
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

  data = BF_Block_GetData(tmpBlock);

  char c = 1; //It is leaf so the first byte of the block is going to be 1
  memcpy(data, &c, sizeof(char));
  data += sizeof(char);

  memcpy(data, &blockNum, sizeof(int)); //Writing the block's id to it
  data += sizeof(int);

  nextPtr = -1; //It is a leaf and the last one
  memcpy(data, &nextPtr, sizeof(int));
  data += sizeof(int);

  recordsNum = 0; //No records yet inserted
  memcpy(data, &recordsNum, sizeof(int));

  BF_Block_SetDirty(tmpBlock);
  CALL_OR_DIE(BF_UnpinBlock(tmpBlock));

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

  int type1,type2,len1,len2, fileDesc;
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

  CALL_OR_DIE(BF_UnpinBlock(tmpBlock));
  BF_Block_Destroy(&tmpBlock);

  
  int file_index = insert_file(type1, len1, type2, len2);
  //check if we have reached the maximum number of files
  if(file_index == -1)
    return AME_MAXFILES;

  insert_bfd(file_index, fileDesc);

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
  /*BF_Block *tmpBlock;
  BF_Block_Init(&tmpBlock);

  void *data = NULL;
  CALL_OR_DIE(BF_GetBlock(openFiles[fileDesc]->bf_desc, 0, tmpBlock));//Getting the first block
  data = BF_Block_GetData(tmpBlock);//and its data

  int type1, len1, type2, len2, targetBlockId;

  data += sizeof(char)*15; //skip the diblus keyword
  //Getting the attr1 and attr2 type and length from the metadata block
  memcpy(&type1, data, sizeof(int));
  data += sizeof(int);
  memcpy(&len1, data, sizeof(int));
  data += sizeof(int);
  memcpy(&type2, data, sizeof(int));
  data += sizeof(int);
  memcpy(&len2, data, sizeof(int));

  CALL_OR_DIE(BF_UnpinBlock(tmpBlock));
  BF_Block_Destroy(&tmpBlock);

  if (type1 == 1)
  {
    targetBlockId = findLeaf(openFiles[fileDesc]->bf_desc, *(int *)value1);
  }else{
    //WE HAVE TO MAKE A FIND LEAF FOR STRINGS AND FLOATS
  }

  BF_Block_Init(&tmpBlock);

  CALL_OR_DIE(BF_GetBlock(openFiles[fileDesc]->bf_desc, targetBlockId, tmpBlock));//Getting the block that we are supposed to insert the new record
  data = BF_Block_GetData(tmpBlock);//and its data

  data += (sizeof(char) + sizeof(int)*2);

  int currRecords, maxRecords;

  memcpy(&currRecords, data, sizeof(int));

  maxRecords = (BF_BLOCK_SIZE - (sizeof(char) + sizeof(int)*3))/(len1 + len2);

  //An oxi gemato apla valto sto telos afou traversareis an gemato parta arxidia mou kai kanta soupa


  CALL_OR_DIE(BF_UnpinBlock(tmpBlock));
  BF_Block_Destroy(&tmpBlock);
*/

  return AME_OK;
}


int AM_OpenIndexScan(int fileDesc, int op, void *value) {
  /*Scan* scan = malloc(sizeof(Scan));
  scan->fileDesc = fileDesc;
	scan->op = op;
	scan->value = value;
	scan->block_num = -1;
	scan->record_num = -1;
  scan->ScanIsOver = false;

	return openScansInsert(scan);*/
  return AME_OK;
}


void *AM_FindNextEntry(int scanDesc) {
  /*scan = openScans[scanDesc];
  file_info file = openFiles[scan->fileDesc];
  int next_block_num;

  //get the next_block_num
  int block_num = scan->block_num;
  if(block_num == -1)
    //if there is no block yet (scan->block_num=-1)
    next_block_num = 0;
  else{
    //get next_block_num from current block
    BF_Block* block;
    BF_Block_Init(&block);
    BF_GetBlock(scan->fileDesc,scan->block_num,block);
    char* data = BF_Block_GetData(block);
    int offset = sizeof(char)+sizeof(int);
    memcpy(&next_block_num,data+offset,sizeof(int));
  }

  //get next block
  if(next_block_num == -1){ //make sure there is a next block
    scan->ScanIsOver = true;
    AM_errno = AME_EOF;
    return NULL;
  }
  BF_Block* next_block;
  BF_GetBlock(scan->fileDesc,next_block_num,next_block);
  char* data = BF_Block_GetData(next_block);

  if(nextRecord == NULL)
    return NULL;

  switch(scan.op){
    case EQUAL:

                if()
  }*/
}

int AM_CloseIndexScan(int scanDesc) {
  return AME_OK;
}


void AM_PrintError(char *errString) {
  printf("%s\n", errString);
}

void AM_Close() {
  delete_files();
}
