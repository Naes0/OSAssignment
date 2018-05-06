#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include "sds.h"

Data* data;

int main(int argc, char const *argv[])
{
   int r = 0;
   int w = 0;
   int t1 = 0;
   int t2 = 0;
   int i = 0;

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



   }
   fclose(data->fp); //closing file pointer

   // final print of the reader and writer counts
   printf("\nreadcount: %d\nwritecount: %d\n", data->readercount, data->writercount);

   //destory threads;

   return 0;
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
