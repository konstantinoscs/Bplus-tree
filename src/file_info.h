#ifndef FILE_INFO_H
#define FILE_INFO_H

extern file_info * openFiles[20];

//file_info keeps all the necessary info about every open file
typdef struct file_info{
  int bf_desc;    //the descriptor of BF level
  int type1;
  int length1;
  int type2;
  int length2;
}file_info;

//insert_file takes the name of a file and inserts it into openFiles
//if openFiles is fulll then -1 is returned
int insert_file(int type1, int length1, int type2, int length2);

//insert_bdf inserts a file_desc file_info
void insert_bfd(int fileDesc, int bf_desc);

void close_file(int i);

#endif