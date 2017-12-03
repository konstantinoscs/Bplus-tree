#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "AM.h"
#include "file_info.h"
#include "BoolType.h"
#include "Scan.h"
#include "stack.h"
#include "HelperFunctions.h"
#include "insert_lib.h"

int AM_errno = AME_OK;
extern int size1;

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
  size1 = len1;
  /*attrMeta.type1 = type1;
  attrMeta.len1 = len1;
  attrMeta.type2 = type2;
  attrMeta.len2 = len2;*/

  int fd;
  int rootInitialized = 0;  //The root is not initialized yet, no record has been inserted
  //temporarily insert the file in openFiles
  int file_index = insert_file(type1, len1, type2, len2, rootInitialized);
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
  //Writing the attr1 and attr2 type and length right after the keyWord in the metadata block and a flag that root has no key yet
  memcpy(data, &type1, sizeof(int));
  data += sizeof(int);
  memcpy(data, &len1, sizeof(int));
  data += sizeof(int);
  memcpy(data, &type2, sizeof(int));
  data += sizeof(int);
  memcpy(data, &len2, sizeof(int));
  data += sizeof(int)*2;
  memcpy(data, &rootInitialized, sizeof(int));

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

  printf("CREATEINDEX len1: %d len2: %d\n", len1, len2 );

  return AME_OK;
}


int AM_DestroyIndex(char *fileName) {
  return AME_OK;
}


int AM_OpenIndex (char *fileName) {
  BF_Block *tmpBlock;
  BF_Block_Init(&tmpBlock);

  int type1,type2,len1,len2, fileDesc, rootId, rootInitialized;
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
  data += sizeof(int);
  memcpy(&rootInitialized, data, sizeof(int));
printf("OPENINDEX len1: %d len2: %d\n", len1, len2 );
  CALL_OR_DIE(BF_UnpinBlock(tmpBlock));
  BF_Block_Destroy(&tmpBlock);


  int file_index = insert_file(type1, len1, type2, len2, rootInitialized);
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
printf("-----------INSERTING-----------\n");
  BF_Block *tmpBlock;
  BF_Block_Init(&tmpBlock);
  void * rootData = NULL;
  int offset = 0;
  int type1, len1, type2, len2;
  type1 = len1 = type2 = len2 = 0;

  //Getting the attr1 and attr2 type and length
  type1 = openFiles[fileDesc]->type1;
  len1 = openFiles[fileDesc]->length1;
  type2 = openFiles[fileDesc]->type2;
  len2 = openFiles[fileDesc]->length2;

  if (!openFiles[fileDesc]->rootInitialized)  //If its the first entry that we insert we have to put it as a key to the root
  {
    CALL_OR_DIE(BF_GetBlock(openFiles[fileDesc]->bf_desc, openFiles[fileDesc]->root_id, tmpBlock));//Getting the root
    rootData = BF_Block_GetData(tmpBlock);//and its data
    offset = sizeof(char) + sizeof(int)*2;
    int currKeys = 1;
    memcpy(rootData + offset, &currKeys, sizeof(int));  //Increasing the number of current keys to root to one
    offset += sizeof(int)*2;
    memcpy(rootData + offset, value1, len1);  //Writing the first value of the new record as a key to the root
    openFiles[fileDesc]->rootInitialized = 1; //The root now is initialized
    BF_Block_SetDirty(tmpBlock);
    CALL_OR_DIE(BF_UnpinBlock(tmpBlock));

    CALL_OR_DIE(BF_GetBlock(openFiles[fileDesc]->bf_desc, 0, tmpBlock));//Getting the metadata block
    rootData = BF_Block_GetData(tmpBlock);//and its data

    offset = sizeof(char)*15 + sizeof(int)*5;
    int rootInitialized = 1;
    memcpy(rootData + offset, &rootInitialized, sizeof(int));

    BF_Block_SetDirty(tmpBlock);
    CALL_OR_DIE(BF_UnpinBlock(tmpBlock));
  }
  //get root block
  BF_GetBlock(openFiles[fileDesc]->bf_desc, 0, tmpBlock);
  char* data = BF_Block_GetData(tmpBlock);
  int rootId;
  memmove(&rootId,data+15*sizeof(char)+4*sizeof(int),sizeof(int));
  BF_UnpinBlock(tmpBlock);
  BF_Block_Destroy(&tmpBlock);
  //initialize a stack to keep track of the path whiel traversing the tree
  Stack *nodesPath;
  create_stack(&nodesPath);
  stack_push(nodesPath,rootId);
  printf("prin insert leaf\n");
  //insert the new record in a leaf block
  insert_leaf_val(value1,value2,fileDesc,nodesPath);
  //clean up
  printf("meta insert leaf\n");
  destroy_stack(nodesPath);
  return AME_OK;
}

/*TOUTHANASI
int AM_InsertEntry(int fileDesc, void *value1, void *value2) {

  void *data1 = NULL;
  void *data2 = NULL;
  void *rootData = NULL;

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

  targetBlockId = findLeaf(fileDesc, value1, nodesPath); //Find the leaf that this value is supposed to be inserted and the path getting there
printf("INSERT type1:%d type2:%d len1: %d len2: %d\n", type1, type2, len1, len2 );

  BF_Block *tmpBlock, *tmpBlock1, *tmpBlock2;
  BF_Block_Init(&tmpBlock);
  BF_Block_Init(&tmpBlock1);
  BF_Block_Init(&tmpBlock2);
  //Getting the block that we are supposed to insert the new record
  CALL_OR_DIE(BF_GetBlock(openFiles[fileDesc]->bf_desc, targetBlockId, tmpBlock1));
  data1 = BF_Block_GetData(tmpBlock1);  //and its data

  memcpy(&nextPtr, data1 + sizeof(char) + sizeof(int), sizeof(int));        // Getting the nextPtr of the targed block
  memcpy(&currRecords, data1 + sizeof(char) + 2*sizeof(int), sizeof(int));  //Getting the num of records that exist in this block
  maxRecords = (BF_BLOCK_SIZE - (sizeof(char) + sizeof(int)*3))/(len1 + len2);

  recordIndex = findRecordPos(data1, fileDesc, value1);  //Get the position it should go

  if (currRecords < maxRecords)
  {
    simpleInsertToLeaf(recordIndex, fileDesc, data1, currRecords, value1, value2);

    BF_Block_SetDirty(tmpBlock1);
    CALL_OR_DIE(BF_UnpinBlock(tmpBlock1));

    if (!openFiles[fileDesc]->rootInitialized)  //If its the first entry that we insert we have to put it as a key to the root
    {
      CALL_OR_DIE(BF_GetBlock(openFiles[fileDesc]->bf_desc, openFiles[fileDesc]->root_id, tmpBlock));//Getting the root
      rootData = BF_Block_GetData(tmpBlock);//and its data
      offset = sizeof(char) + sizeof(int)*2;
      int currKeys = 1;
      memcpy(rootData + offset, &currKeys, sizeof(int));  //Increasing the number of current keys to root to one
      offset += sizeof(int)*2;
      memcpy(rootData + offset, value1, len1);  //Writing the first value of the new record as a key to the root
      openFiles[fileDesc]->rootInitialized = 1; //The root now is initialized
      BF_Block_SetDirty(tmpBlock);
      CALL_OR_DIE(BF_UnpinBlock(tmpBlock));

      CALL_OR_DIE(BF_GetBlock(openFiles[fileDesc]->bf_desc, 0, tmpBlock));//Getting the metadata block
      rootData = BF_Block_GetData(tmpBlock);//and its data

      offset = sizeof(char)*15 + sizeof(int)*5;
      int rootInitialized = 1;
      memcpy(rootData + offset, &rootInitialized, sizeof(int));

      BF_Block_SetDirty(tmpBlock);
      CALL_OR_DIE(BF_UnpinBlock(tmpBlock));
    }

  }else{   //If this block is full we are going to split it and divide its data
    //Allocating and initializing the new block
    CALL_OR_DIE(BF_AllocateBlock(openFiles[fileDesc]->bf_desc, tmpBlock2));
    int blockId;
    CALL_OR_DIE(BF_GetBlockCounter(openFiles[fileDesc]->bf_desc, &blockId));
    blockId--;

    offset = (sizeof(char) + sizeof(int));
    memcpy(data1 + offset, &blockId, sizeof(int));  //Set as next of the old block the new block

    data2 = BF_Block_GetData(tmpBlock2);

    //Inserting data to the new block, arguments: 1 means a leaf, blockId is its id,
    //nextPtr means the next block of the new block is the one that was next to the old block, 0 records yet inserted
    blockMetadataInit(data2, 1, blockId, nextPtr, 0); //STEFANIDI EDW EISAI

    int numRecToNew = maxRecords/2+1;
    int numRecToOld = maxRecords-numRecToNew;
    bool goesToNew = 1; //if 0 go to old block
    if (recordIndex < numRecToOld)
      goesToNew = 0;  //go to old

    offset = sizeof(char) + 3*sizeof(int);
    //will the new record go to the old or the new block?
    if (!goesToNew){  //If the new record goes to the old block
      memcpy(data2 + offset, data1+offset+(numRecToOld-1)*(len1+len2),numRecToNew*(len1+len2));
      //Updating the block counters of the old and the new block
      offset = sizeof(char) + sizeof(int)*2;
      memcpy(data1 + offset, &numRecToOld, sizeof(int));
      memcpy(data2 + offset, &numRecToNew, sizeof(int));
      //inserting the new record in the old block
      recordIndex = findRecordPos(data1, fileDesc, value1);  //Get the position it should go again in case something changed
      simpleInsertToLeaf(recordIndex, fileDesc, data1, numRecToOld-1, value1, value2); //Make a simple insertion to the old block
    }
    else{           //If the new record goes to the new block
      memcpy(data2 + offset, data1+offset+numRecToOld*(len1+len2),numRecToNew*(len1+len2));
      //Updating the block counters of the old and the new block
      offset = sizeof(char) + sizeof(int)*2;
      memcpy(data1 + offset, &numRecToOld, sizeof(int));
      memcpy(data2 + offset, &numRecToNew, sizeof(int));
      //inserting the new record to the new block
      recordIndex = findRecordPos(data2, fileDesc, value1);  //Get the position it should go again in case something changed
      simpleInsertToLeaf(recordIndex, fileDesc, data2, numRecToNew-1, value1, value2);  //Make a simple insertion to the old block
    }
     //Take the first attribute of the new block as a key to its parent
    char *newKey = malloc(len1);
    memcpy(newKey, data2+sizeof(char)+3*sizeof(int), len1); //newKey will be the 1st key of the new(right) block

    /*int currRecords1, currRecords2;
    char *lastKey = malloc(len1);

    offset = sizeof(char) + sizeof(int)*2;
    memcpy(&currRecords1, data1 + offset, sizeof(int));
    memcpy(&currRecords2, data2 + offset, sizeof(int));

    offset += (sizeof(int) + (currRecords1 - 1)*(len1 + len2));
    memcpy(lastKey, data1 + offset, len1);

    if (keysComparer(lastKey, newKey, EQUAL, type1))
    {
      int sameKeys1 = sameKeysCount(data1, lastKey, len1, type1, currRecords1);
      int sameKeys2 = sameKeysCount(data2, newKey, len1, type1, currRecords2);

      if ((maxRecords - currRecords1) >= (sameKeys1 + sameKeys2))
      {
        offset = sizeof(char) + sizeof(int)*3 + currRecords1*(len1 + len2);
        offset2 = sizeof(char) + sizeof(int)*3;
        memcpy(data1 + offset, data2 + offset2, sameKeys2*(len1 + len2));
        currRecords1 += sameKeys2;
        currRecords2 -= sameKeys2;
        memcpy(data2 + offset2, data2 + offset2 + sameKeys2*(len1 +len2), currRecords2*(len1 + len2));
        offset = sizeof(char) + sizeof(int)*2;
        offset2 = sizeof(char) + sizeof(int)*2;
        memcpy(data1 + offset, currRecords1, sizeof(int));
        memcpy(data2 + offset2, currRecords2, sizeof(int));
      }else
      {

      }

    }


    BF_Block_SetDirty(tmpBlock1);
    CALL_OR_DIE(BF_UnpinBlock(tmpBlock1));

    BF_Block_SetDirty(tmpBlock2);
    CALL_OR_DIE(BF_UnpinBlock(tmpBlock2));

    //Inserting to the index node(s) the first value of the new block, given the files descriptor, the path and the id of the new leaf
    insert_index_val(newKey, fileDesc, nodesPath, blockId);
    free(newKey);

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

  destroy_stack(nodesPath);

  //CALL_OR_DIE(BF_UnpinBlock(tmpBlock));
  BF_Block_Destroy(&tmpBlock);
  BF_Block_Destroy(&tmpBlock1);
  BF_Block_Destroy(&tmpBlock2);

  return AME_OK;
}*/


int AM_OpenIndexScan(int fileDesc, int op, void *value) {
	return openScansInsert(ScanInit(fileDesc,op,value));
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
                  scan->block_num = findLeaf(scan->fileDesc,scan->value,NULL);
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
                  //make sure this block is not empty
                  int num_of_records;
                  memmove(&num_of_records,data+sizeof(bool)+2*sizeof(int),sizeof(int));
                  while(num_of_records == 0){
                    ScanNextRecord(scan,&block,&data);  //next block
                    memmove(&num_of_records,data+sizeof(bool)+2*sizeof(int),sizeof(int)); //update num_of_records
                  }
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
                  //make sure this block is not empty
                  int num_of_records;
                  memmove(&num_of_records,data+sizeof(bool)+2*sizeof(int),sizeof(int));
                  while(num_of_records == 0){
                    ScanNextRecord(scan,&block,&data);  //next block
                    memmove(&num_of_records,data+sizeof(bool)+2*sizeof(int),sizeof(int)); //update num_of_records
                  }
                  //find the first record of this block
                  scan->record_num = 0;
                  void* recordAttr1 = data+sizeof(char)+3*sizeof(int);
                  //is this record less_than?SVISE
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
                  scan->block_num = findLeaf(scan->fileDesc,scan->value,NULL);
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
                  //make sure this block is not empty
                  int num_of_records;
                  memmove(&num_of_records,data+sizeof(bool)+2*sizeof(int),sizeof(int));
                  while(num_of_records == 0){
                    ScanNextRecord(scan,&block,&data);  //next block
                    memmove(&num_of_records,data+sizeof(bool)+2*sizeof(int),sizeof(int)); //update num_of_records
                  }
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
                  scan->block_num = findLeaf(scan->fileDesc,scan->value,NULL);
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
