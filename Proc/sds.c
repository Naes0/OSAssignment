#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include "sds.h"

Semaphores* sems;
Data* data;

int main(int argc, char const *argv[])
{

   int r = 0;
   int w = 0;
   int t1 = 0;
   int t2 = 0;
   int i = 0;
   int test; 
   FileDescriptors fileDes;

   if (argc != 5)
   {
      printf("\nIncorrect number input arguments");
      printf("\nEnter 4 numbers for reader, writers, t1 and t2 respectively");
   }
   else
   {
      // assigning number of reader and writers + timers t1 and t2
      // from command line arguments[]
      r = atoi(argv[1]);
      w = atoi(argv[2]);
      t1 = atoi(argv[3]);
      t2 = atoi(argv[4]);
      printf("r = %d, w = %d, t1 = %d, t2 = %d\n", r, w, t1, t2);

      createSharedMemory(&fileDes);
      initSemaphores();

      //initialise shared memory
      data->readercount = 0;
      data->writercount = 0;
      data->writerDataIndex = 0;
      data->dataBufferIndex = 0;
      data->readersRun = r;

      // reading from shared_data into array
      data->fp = fopen("shared_data", "r");
      if (data->fp == NULL)
      {
         printf("Error: Could not open file\n");
      }
      else
      {
         while (feof(data->fp) == 0)
         {
            fscanf(data->fp, "%d,", &data->sharedData[i]);
            i++;
         }
         if(ferror(data->fp))
         {
            perror("Error while reading.");
         }
         fclose(data->fp);
      }
      data->fp = fopen("sim_out", "w");

      for(int i = 0; i < 20; i++)
      {
         data->dataBuffer[i] = 0;
      }

      // get parent id
      pid_t parentPid = getpid();
      createProccess(parentPid, r, w, t1, t2);

   }
   fclose(data->fp);
   printf("\nreadcount: %d\nwritecount: %d\n", data->readercount, data->writercount);
   destroySemaphores();
   destroySharedMemory(&fileDes);

   return 0;
}

void createProccess(pid_t parentPid, int r, int w, int t1, int t2)
{

   // READER SECTION
   //only the parent produces new process
   for (int i = 0; i < r; i++)
   {
      if (getpid() == parentPid)
      {
         fork();
      }
   }

   if (getpid() != parentPid)
   {
      reader(t1, r);
   }

   //WRITER SECTION
   //only the parent produces new process
   for (int i = 0; i < w; i++)
   {
      if (getpid() == parentPid)
      {
         fork();
      }
   }

   if (getpid() != parentPid)
   {
      printf("\nI am writer: %d", (int) getpid());
      writer(t2);
   }

   while (wait(NULL)> 0)
   {
      printf("\n%d: waiting....\n", getpid());
   }
   printf("\n%d: done waiting!\n ", getpid());

}

void reader(int timer1, int r)
{
   int readSize = 0; //amount of data read
   int readData = 0; //read data cant be zero so this can be intiialised
   int readDataIndex = 0;
   int value = 0;
   printf("\nI am reader: %d", (int) getpid());

   while(readSize != 100)
   {
      sem_wait(&sems->semReader); //lock reader 0
         data->readercount++;
         if(data->readercount == 1)
         {
            sem_wait(&sems->semWriter); //lock writer 0
         }
      sem_post(&sems->semReader); //unlock reader 1

         //reading is preformed
         if(data->dataBufferIndex == 20) //if the writers have filled the data buffer
         {
            if (data->readersRun == r)
            {
               for(int i = 0; i < 20; i++)
               {
                  readData = data->dataBuffer[i];
                  printf("\nReader: %d    Read Data: %d\n", getpid(), readData); //print read data
                  readSize++;
                  sleep(timer1);
               }
               sem_wait(&sems->semDBuffer);
                  data->readersRun--;
               sem_post(&sems->semDBuffer);
            }
         }
         //end reading

         //reseting buffer
         sem_wait(&sems->semDBuffer);
         if(data->readersRun == 0)
         {
            for (int i = 0; i < 20; i++)
            {
               data->dataBuffer[i] = 0;
            }
            data->readersRun = r;
            data->dataBufferIndex = 0;
         }
         sem_post(&sems->semDBuffer);


      sem_wait(&sems->semReader); //locks reader 0
         data->readercount--;
         if(data->readercount == 0)
         {
            sem_post(&sems->semWriter); //unlock writer 1
         }
         sem_post(&sems->semReader); //unlock reader 1
   }
   printf("readSize: %d\n", readSize);
   sem_wait(&sems->semSharedData);
      fprintf(data->fp, "\nReader-%d has finished reading %d pieces of data from the data_buffer\n", getpid(), readSize);
   sem_post(&sems->semSharedData);
   exit(0);

}

void writer(int timer2)
{
   int writeSize = 0; //amount of data written
   int writeData = 0; //read data cant be zero so this can be intiialised

   sem_wait(&sems->semWriter);
   while(data->writerDataIndex != 100)
   {
      sem_post(&sems->semWriter);
            sem_wait(&sems->semWriter );
               if(data->dataBufferIndex < 20) //buffer is no full
               {
                  writeData = data->sharedData[data->writerDataIndex]; //get data from shared data
                  data->dataBuffer[data->dataBufferIndex] = writeData;
                  printf("\nWriter: %d    Written Data: %d    DataBuffer[%d]: %d \n", getpid(), writeData, data->dataBufferIndex, data->dataBuffer[data->dataBufferIndex]);   //print write data
                  data->writercount++;
                  data->writerDataIndex++;
                  data->dataBufferIndex++;
                  writeSize++;
                  sleep(timer2);
               }
            sem_post(&sems->semWriter);
      //printf("\nwriterSize: %d", writeSize);
   }
   sem_wait(&sems->semSharedData);
      fprintf(data->fp, "\nWriter-%d has finished writing %d pieces of data to the data_buffer\n", getpid(), writeSize);
   sem_post(&sems->semSharedData);
   exit(0);
}

//Creates shared memory blocks to allow processes to access shared data
void createSharedMemory(FileDescriptors* fileDes)
{
   //creates shared memory which then points to its respective FileDescriptors.
   fileDes->fdData = shm_open("sharedMemData", O_RDWR|O_CREAT, 0666);
   fileDes->fdSemaphores = shm_open("sharedMemSemaphores", O_RDWR|O_CREAT, 0666);

   //set size of each shared shared memory
   ftruncate(fileDes->fdData, (off_t)sizeof(Data));
   ftruncate(fileDes->fdSemaphores, (off_t)sizeof(Semaphores));

   //Link shared memory to objects
   sems = (Semaphores*)mmap(NULL, sizeof(Semaphores), PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, fileDes->fdSemaphores, 0);
   data= (Data*)mmap(NULL, sizeof(Data), PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, fileDes->fdData, 0);
}

void destroySharedMemory(FileDescriptors *fileDes)
{
   //unmaping semaphores and shared data
   munmap(sems, sizeof(Semaphores));
   munmap(data, sizeof(Data));

   //unlinking semaphores and shared data
   shm_unlink("sharedMemData");
   shm_unlink("sharedMemSemaphores");
}

void initSemaphores()
{
   sem_init(&sems->semSharedData, 1, 1);
   sem_init(&sems->semDBuffer, 1, 1);
   sem_init(&sems->semReader, 1, 1);
   sem_init(&sems->semWriter, 1, 1);
   sem_init(&sems->semExtra, 1, 1);
}

void destroySemaphores()
{
   sem_destroy(&sems->semSharedData);
   sem_destroy(&sems->semDBuffer);
   sem_destroy(&sems->semReader);
   sem_destroy(&sems->semWriter);
   sem_destroy(&sems->semExtra);
}
