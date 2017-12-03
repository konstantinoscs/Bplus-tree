#ifndef SCAN_H
#define SCAN_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "BoolType.h"
#include "file_info.h"
#include "bf.h"

typedef struct Scan {
	int fileDesc;			//the file that te scan refers to
	int block_num;		//last block that was checked
	int record_num;		//last record that was checked
	int op;			//the operation
	void *value;			//the target value
  bool ScanIsOver;
  void* return_value; //gets allocated first time AM_FindNextEntry is called, and freed when ScanIsOver. Holds the last value returned. At the start its NULL.
}Scan;

#define MAX_SCANS 20
Scan* openScans[MAX_SCANS];  //This is where open Scans are saved. The array is initialized with NULL's.

int openScansInsert(Scan* scan);  //inserts a Scan in openScans[] if possible and returns the position
int openScansFindEmptySlot();     //finds the first empty slot in openScans[]
bool openScansFull();             //checks

//allocate and initialize a Scan, return a pointer to it
Scan* ScanInit(int fileDesc,int op,void* value);

//look at the next record and return its offset inside the data, makes sure you update the block if you got out of its bounds and that blocks data
int ScanNextRecord(Scan*,BF_Block** block_ptr,char** data);

//gets fd for openFiles and checks if this file has an open Scan
bool hasOpenScan(int fd);

#endif
