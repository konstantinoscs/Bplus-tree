#include <stdlib.h>

#include "file_info.h"

int insert_file(int type1, int length1, int type2, int length2){
  for(int i=0; i<20; i++){
    //if a spot is free put the info in it and return
    //its index
    if(openFiles[i] == NULL ){
      openFiles[i] = malloc(sizeof(file_info));
      openFiles[i]->type1 = type1;
      openFiles[i]->length1 = length1;
      openFiles[i]->type2 = type2;
      openFiles[i]->length2 = length2;
      return i;
    }
  }
  return -1;
}

void insert_bfd(int fileDesc, int bf_desc){
  openFiles[fileDesc]->bf_desc = bf_desc;
}

//close_file removes a file with index i from openFiles
void close_file(int i){
  free(openFiles[i]);
  openFiles[i] == NULL;
}

void delete_files(){
	for(int i=0; i<20; i++)
		close_file(i);
}