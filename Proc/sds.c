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

      //opening file writer to write to the file sim_out
      data->fp = fopen("sim_out", "w");

      //initialising data buffer to zero
      for(int i = 0; i < 20; i++)
      {
         data->dataBuffer[i] = 0;
      }

      // get parent id
      pid_t parentPid = getpid();

      // forking and creating processes for the appropriate number of readers and writers
      createProccess(parentPid, r, w, t1, t2);

   }
   fclose(data->fp); //closing file pointer

   // final print of the reader and writer counts
   printf("\nreadcount: %d\nwritecount: %d\n", data->readercount, data->writercount);
   //destroying shared memory and semaphores
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

   if (getpid() != parentPid) //child processes call reader function
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
      writer(t2); //child processes call writer function
   }

   while (wait(NULL)> 0) //making parent wait for childern to terminate
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

   while(readSize != 100) // while each reader process has not read 100 times
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
            if (data->readersRun == r) // if the current reader processes running equal the total readers created
            {
               for(int i = 0; i < 20; i++) // then read from data buffer
               {
                  readData = data->dataBuffer[i];
                  printf("\nReader: %d    Read Data: %d\n", getpid(), readData); //print read data
                  readSize++;
                  sleep(timer1);
               }
               sem_wait(&sems->semDBuffer);
                  data->readersRun--; // decrease running readers since reading has finished
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
               data->dataBuffer[i] = 0; //reset buffer to 0;
            }
            data->readersRun = r;// sets running readers back to enter value for the next set of 20 elements
            data->dataBufferIndex = 0; //setting index back to 0 so the writer starts from element 0 when writing to buffer
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
   printf("\nreadSize: %d\n", readSize);
   sem_wait(&sems->semSharedData);
      fprintf(data->fp, "\nReader-%d has finished reading %d pieces of data from the data_buffer\n", getpid(), readSize); //printing out to sim_out
   sem_post(&sems->semSharedData);
   exit(0); //terminate process
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
                  data->dataBuffer[data->dataBufferIndex] = writeData; //write the recieved data to the data buffer
                  printf("\nWriter: %d    Written Data: %d    DataBuffer[%d]: %d \n", getpid(), writeData, data->dataBufferIndex, data->dataBuffer[data->dataBufferIndex]);   //print write data
                  data->writercount++; //increment counters
                  data->writerDataIndex++;
                  data->dataBufferIndex++;
                  writeSize++;
                  sleep(timer2);
               }
            sem_post(&sems->semWriter);
   }
   sem_wait(&sems->semSharedData);
      fprintf(data->fp, "\nWriter-%d has finished writing %d pieces of data to the data_buffer\n", getpid(), writeSize); //print out to sim_out
   sem_post(&sems->semSharedData);
   exit(0);
}

//Creates  memory blocks to allow processes to access shared data
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
}

void destroySemaphores()
{
   sem_destroy(&sems->semSharedData);
   sem_destroy(&sems->semDBuffer);
   sem_destroy(&sems->semReader);
   sem_destroy(&sems->semWriter);
}
