#include "HelperFunctions.h"

/***************************************************************************************************************************
**************INSERT AND SCAN*********************************************************************************************************
******************************************************************************************************************************/

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
//value1,2 the values to be inserted in the block
void simpleInsertToLeaf(int recordIndex, int fd, void* data, int currRecords, void *value1, void *value2){
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
