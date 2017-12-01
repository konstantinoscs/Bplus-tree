
#include "Scan.h"

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

//look at the next record and return its offset inside the data, makes sure you update the block if you got out of its bounds and that blocks data
int ScanNextRecord(Scan* scan, BF_Block** block_ptr,char** data_ptr){
	scan->record_num++;
	int* num_of_records;
	memcpy(num_of_records,*data_ptr+sizeof(char)+2*sizeof(int),sizeof(int));
	//is the next record in this or the next block?
	if(scan->record_num >= *num_of_records){  //if its in the next block
		int* next_block_num;
		memcpy(next_block_num,*data_ptr+sizeof(char)+sizeof(int),sizeof(int));
		if(*next_block_num == -1){  //if there is no next block, end Scan
			free(scan->return_value);
			scan->return_value = NULL;
			scan->ScanIsOver = true;
			BF_UnpinBlock(*block_ptr);
			BF_Block_Destroy(block_ptr);
			return NO_NEXT_BLOCK;
		}
		//get rid of old block
		BF_UnpinBlock(*block_ptr);
		BF_Block_Destroy(block_ptr);
		//open the next block
		BF_Block_Init(block_ptr);
		file_info* file = openFiles[scan->fileDesc];
		BF_GetBlock(file->bf_desc,*next_block_num,*block_ptr);
		*data_ptr = BF_Block_GetData(*block_ptr);
		//update scan counters
		scan->block_num = *next_block_num;
		scan->record_num = 0;
		return sizeof(char)+3*sizeof(int);	//return the offset of the first record of the next block
	}
	else{	//return the offset of the next record (in this block)
		file_info* file = openFiles[scan->fileDesc];
		return sizeof(char)+3*sizeof(int)+(scan->record_num-1)*(sizeof(file->length1)+sizeof(file->length2));
	}
}

int getRecord(void* data, int fileDesc, int pos){  //return the offset of the record at pos
  file_info* file = openFiles[fileDesc];
  //check if there is a record at pos
  int* num_of_records;
	memcpy(num_of_records,data+sizeof(char)+2*sizeof(int),sizeof(int));
  if(pos >= *num_of_records)  //if not: abort
    return -1;
  else{
    int offset = sizeof(char)+3*sizeof(int);
    for(int i=0; i<pos; i++){
      offset += sizeof(file->length1)+sizeof(file->length2);
    }
    return offset;
  }
}
