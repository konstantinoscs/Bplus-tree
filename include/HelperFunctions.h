#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include "BoolType.h"
#include "stack.h"
#include "bf.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "file_info.h"
#include "defn.h"

#define AME_OK 0
#define AME_EOF -1
#define AME_WRONGARGS -2
#define AME_MAXFILES -3
#define AME_RECORD_NOT_FOUND -4 //while scanning the record you asked for was not found by its key

#define EQUAL 1
#define NOT_EQUAL 2
#define LESS_THAN 3
#define GREATER_THAN 4
#define LESS_THAN_OR_EQUAL 5
#define GREATER_THAN_OR_EQUAL 6

#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      printf("Error\n");      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }                           \



//openFiles holds the info of open files in the appropriate index
file_info * openFiles[20];

/***************************************************************************************************************************
**************INSERT AND SCAN*********************************************************************************************************
******************************************************************************************************************************/

//Takes the search key(target key) and the traversing key(tmp key), the operation (EQUAL, LESS_THAN etc) that we want to apply on the keys and the keytype
//and returns if the operation is true or false
//is targetkey op tmpkey?
bool keysComparer(void *targetKey, void *tmpKey, int operation, int keyType);

//Returning the stack full with the path to the leaf and the id of the leaf
//that has the key we are looking for or should have it at least. If called with null leafPath we didnt need
//to keep tha path in this function call
int findLeaf(int fd, void *key, Stack **leafPath);

//Returning the id of the most left leaf
int findMostLeftLeaf(int fd);

//findRecord finds the position [0-n) of the record that has attr1 as value1 if it exists, otherwise the position it would be
int findRecordPos(void * data, int fd, void * value1);

//RecordIndex->the index the new record should go [0-n), fd our openFiles descriptor, currRecords the amount of existing records in this block
//value1,2 tha values to be inserted in the block
void simpleInsertToLeaf(int recordIndex, int fd, void *data, int currRecords, void *value1, void *value2);

/***************************************************************************************************************************
**************CREATE*********************************************************************************************************
******************************************************************************************************************************/
//Checks if the attributes are correct and if they agreee with the length given for them
int typeChecker(char attrType, int attrLength, int *type, int *len);

//Initializing a new blocks metadata
void blockMetadataInit(void *data, bool isLeaf, int blockId, int nextPtr, int recordsNum);

//Returning how many keys are equal to the target key
int sameKeysCount(void *data, void *targetkey, int length, int type, int currRecords);


#endif
