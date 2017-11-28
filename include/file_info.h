#ifndef FILE_INFO_H
#define FILE_INFO_H



//file_info keeps all the necessary info about every open file
typedef struct file_info{
  int bf_desc;    //the descriptor of BF level
  int type1;
  int length1;
  int type2;
  int length2;
  int root_id;
}file_info;

extern file_info * openFiles[20];

//insert_file takes the name of a file and inserts it into openFiles
//if openFiles is fulll then -1 is returned
int insert_file(int type1, int length1, int type2, int length2);

//insert_bdf inserts a file_desc file_info
void insert_bfd(int fileDesc, int bf_desc);

void insert_root(int fileDesc, int root_id);

void close_file(int i);

void delete_files();

#endif