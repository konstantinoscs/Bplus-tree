#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defn.h"
#include "AM.h"

#define NUM_OF_INSERTS 10

void main(void){
  AM_Init();
  AM_CreateIndex("TestBase", INTEGER, 4, INTEGER, sizeof(int));
  int fd = AM_OpenIndex("TestBase");

  //INSERT
  for(int i=0; i<NUM_OF_INSERTS; i++){
    /*string
    char str[5];
    sprintf(str, "%d", i);
    printf("(((%s)))", str);

    */
    AM_InsertEntry(fd,(void*) &i, (void*) &i);
  }
  //SCAN
  int value1=1;
  int scanDesc = AM_OpenIndexScan(fd, EQUAL, &value1);
  int* value2 = AM_FindNextEntry(scanDesc);
  printf("%d\n", *value2);


  AM_CloseIndex(fd);
}
