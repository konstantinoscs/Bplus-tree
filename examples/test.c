#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defn.h"
#include "AM.h"

void main(void){
  AM_Init();
  AM_CreateIndex("TestBase", STRING, 5, INTEGER, sizeof(int));
  int fd = AM_OpenIndex("TestBase");


  for(int i=0; i<10000; i++){
    /*string
    char str[5];
    sprintf(str, "%d", i);
    printf("(((%s)))", str);

    */


    AM_InsertEntry(fd,(void*) &i, (void*) &i);
  }


  AM_CloseIndex(fd);
}
