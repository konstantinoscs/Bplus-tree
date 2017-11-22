#include "AM.h"
#include <stdlib.h>

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
