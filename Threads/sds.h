#include <stdio.h>

typedef struct Data
{
   int sharedData[100];
   int dataBuffer[20];
   int readercount; //number of readers
   int writercount; // number of writers
   int writerDataIndex; // number of written dataBuffer
   int dataBufferIndex; //boolean read all of shared data
   int readersRun; // boolean writen all of shared data
   FILE* fp;
}Data;

void* reader(void*);
void* writer(void*);
void produceThreads();
