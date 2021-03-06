#ifndef AM_H_
#define AM_H_

/* Error codes */

extern int AM_errno;

#define AME_OK 0
#define AME_EOF -1
#define AME_WRONGARGS -2
#define AME_MAXFILES -3
#define AME_RECORD_NOT_FOUND -4 //while scanning the record you asked for was not found by its key
#define HAS_OPEN_SCAN -5  //if the file you are tryint to close has an openScan in openScans (even if the scan is over)
#define NO_NEXT_BLOCK -6  //while scanning if there is no more blocks to scan


#define EQUAL 1
#define NOT_EQUAL 2
#define LESS_THAN 3
#define GREATER_THAN 4
#define LESS_THAN_OR_EQUAL 5
#define GREATER_THAN_OR_EQUAL 6


#include "bf.h"
#include "defn.h"
#include "BoolType.h"

/*
typedef struct BlockMetadata
{
  bool isLeaf; //0 if its an inside node 1 if it is leaf (sizeof(char))
  int blockId; //The unique id of this block
  int nextPtr; //-2 if its an inside node, #>0 if its is leaf, -1 if its the last leaf
  int recordsNum; // The amount of current records, either its keys or data
} BlockMetadata;

typedef struct firstBlock
{
  char anagnwristiko[15];   //DIBLU$
  int type1; //Type of the first(key) attribute. 1 for int 2 for float 3 for string
  int len1; //Length of the first attribute
  int type2; //Type of the second attribute. 1 for int 2 for float 3 for string
  int len2; //Length of the second attribute
  int rootID //The block Id of the root
  int rootInitialized //The root is not initialized yet, no record has been inserted
}firstBlock;*/

bool keysComparer(void *targetKey, void *tmpKey, int operation, int keyType);

void AM_Init( void );


int AM_CreateIndex(
  char *fileName, /* όνομα αρχείου */
  char attrType1, /* τύπος πρώτου πεδίου: 'c' (συμβολοσειρά), 'i' (ακέραιος), 'f' (πραγματικός) */
  int attrLength1, /* μήκος πρώτου πεδίου: 4 γιά 'i' ή 'f', 1-255 γιά 'c' */
  char attrType2, /* τύπος πρώτου πεδίου: 'c' (συμβολοσειρά), 'i' (ακέραιος), 'f' (πραγματικός) */
  int attrLength2 /* μήκος δεύτερου πεδίου: 4 γιά 'i' ή 'f', 1-255 γιά 'c' */
);


int AM_DestroyIndex(
  char *fileName /* όνομα αρχείου */
);


int AM_OpenIndex (
  char *fileName /* όνομα αρχείου */
);


int AM_CloseIndex (
  int fileDesc /* αριθμός που αντιστοιχεί στο ανοιχτό αρχείο */
);


int AM_InsertEntry(
  int fileDesc, /* αριθμός που αντιστοιχεί στο ανοιχτό αρχείο */
  void *value1, /* τιμή του πεδίου-κλειδιού προς εισαγωγή */
  void *value2 /* τιμή του δεύτερου πεδίου της εγγραφής προς εισαγωγή */
);


int AM_OpenIndexScan(
  int fileDesc, /* αριθμός που αντιστοιχεί στο ανοιχτό αρχείο */
  int op, /* τελεστής σύγκρισης */
  void *value /* τιμή του πεδίου-κλειδιού προς σύγκριση */
);


void *AM_FindNextEntry(
  int scanDesc /* αριθμός που αντιστοιχεί στην ανοιχτή σάρωση */
);


int AM_CloseIndexScan(
  int scanDesc /* αριθμός που αντιστοιχεί στην ανοιχτή σάρωση */
);


void AM_PrintError(
  char *errString /* κείμενο για εκτύπωση */
);

void AM_Close();


#endif /* AM_H_ */
