#include "AM.h"

typdef struct scan {
	int fileDesc;			//the file that te scan refers to
	int block_num;		//last block that was checked
	int op;						//the operation
	void *value;			//the value we target
}scan;

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
