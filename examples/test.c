#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defn.h"
#include "AM.h"

void main(void){
  AM_Init();
  AM_CreateIndex("TestBase", INTEGER, sizeof(int), INTEGER, sizeof(int));
  int fd = AM_OpenIndex("TestBase");

  for(int i=0; i<1000; i++){
    printf("(((%d)))", i);
    AM_InsertEntry(fd,(void*) &i, (void*) &i);
  }
  AM_CloseIndex(fd);
}
