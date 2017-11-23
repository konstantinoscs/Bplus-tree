#include "AM.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      printf("Error\n");      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }



int openFiles[20];

/************************************************
**************Scan*******************************
*************************************************/

Scan* openScans[20];

void openScansInsert(Scan* scan){
	int slot = openScansFindEmptySlot();
	openScans[slot] = scan;
}

int openScansFindEmptySlot(){
	for(int i=0; i<20; i++)
	return i;
		if(openScans[i] == NULL)
	return i;
}

bool openScansFull(){
	if(openScansFindEmptySlot() == 20)
	return true;
	return false;
}

typdef struct Scan {
	int fileDesc;			//the file that te scan refers to
	int block_num;		//last block that was checked
	int record_num;		//last record that was checked
	int op;			//the operation
	void *value;			//the target value
}Scan;

bool ScanInit(Scan* scan, int fileDesc, int op, void* value){
	scan = malloc(sizeof(Scan));
	scan->fileDesc = fileDesc;
	scan->op = op;
	scan->value = value;
	scan->block_num = -1;
	scan->record_num = -1;

	if(openScansFull() != true){	//make sure array there is space for one more scan
		openScansInsert[scan];
	}
	else{
		free(scan);
		return false;
	}
}

/************************************************
**************Create*******************************
*************************************************/





/***************************************************
***************AM_Epipedo***************************
****************************************************/

int AM_errno = AME_OK;

void AM_Init() {
  //comment
	return;
}


int AM_CreateIndex(char *fileName,
	               char attrType1,
	               int attrLength1,
	               char attrType2,
	               int attrLength2) {

  int type1,type2,len1,len2;
  //attributesMetadata attrMeta;

  if (attrType1 == INTEGER)
  {
    type1 = 1; //If type1 is equal to 1 the first attribute is int
  }else if (attrType1 == FLOAT)
  {
    type1 = 2; //If type1 is equal to 2 the first attribute is float
  }else if (attrType1 == STRING)
  {
    type1 = 3; //If type1 is equal to 3 the first attribute is string
  }else{
    return AME_WRONGARGS;
  }
  len1 = attrLength1;

  if (attrType2 == INTEGER)
  {
    type2 = 1; //If type2 is equal to 1 the second attribute is int
  }else if (attrType2 == FLOAT)
  {
    type2 = 2; //If type2 is equal to 2 the second attribute is float
  }else if (attrType2 == STRING)
  {
    type2 = 3; //If type2 is equal to 3 the second attribute is string
  }else{
    return AME_WRONGARGS;
  }
  len2 = attrLength2;

  /*attrMeta.type1 = type1;
  attrMeta.len1 = len1;
  attrMeta.type2 = type2;
  attrMeta.len2 = len2;*/

  BF_Block *tmpBlock;
  BF_Block_Init(&tmpBlock);

  int fd;
  CALL_OR_DIE(BF_CreateFile(fileName));
  CALL_OR_DIE(BF_OpenFile(fileName, &fd));

  char *data;
  char keyWord[15];
  CALL_OR_DIE(BF_AllocateBlock(fd, tmpBlock));//Allocating the first block that will host the metadaτa

  data = BF_Block_GetData(tmpBlock);
  strcpy(keyWord,"DIBLU$");
  memcpy(data, keyWord, sizeof(char)*(strlen(keyWord) + 1));//Copying the key-phrase DIBLU$ that shows us that this is a B+ file
  data += sizeof(char)*(strlen(keyWord) + 1);
  memcpy(data, &type1, sizeof(int));
  data += sizeof(int);
  memcpy(data, &len1, sizeof(int));
  data += sizeof(int);
  memcpy(data, &type2, sizeof(int));
  data += sizeof(int);
  memcpy(data, &len2, sizeof(int));
  

  BF_Block_SetDirty(tmpBlock);
  CALL_OR_DIE(BF_UnpinBlock(tmpBlock));

  BF_Block_Destroy(&tmpBlock);
  CALL_OR_DIE(BF_CloseFile(fd));
  return AME_OK;
}


int AM_DestroyIndex(char *fileName) {
  return AME_OK;
}


int AM_OpenIndex (char *fileName) {
  return AME_OK;
}


int AM_CloseIndex (int fileDesc) {
  return AME_OK;
}


int AM_InsertEntry(int fileDesc, void *value1, void *value2) {
  return AME_OK;
}


int AM_OpenIndexScan(int fileDesc, int op, void *value) {
	Scan scan;
	ScanInit(&scan,fileDesc,op,value);

  return AME_OK;
}


void *AM_FindNextEntry(int scanDesc) {

}


int AM_CloseIndexScan(int scanDesc) {
  return AME_OK;
}


void AM_PrintError(char *errString) {

}

void AM_Close() {

}
