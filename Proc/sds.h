#include <semaphore.h>

typedef struct Semaphores
{
   sem_t semSharedData;
   sem_t semDBuffer;
   sem_t semReader;
   sem_t semWriter;
}Semaphores;

typedef struct FileDescriptors
{
   int fdData;
   int fdSemaphores;
}FileDescriptors;

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

void reader(int t1, int r);
void writer(int t2);
void destroySemaphores();
void initSemaphores();
void createSharedMemory(FileDescriptors* fileDes);
void destroySharedMemory(FileDescriptors *fileDes);
void createProccess(pid_t parentPid, int r, int w, int t1, int t2);
