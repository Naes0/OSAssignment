#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include "sds.h"

Data data;
int r;
int w;
int timer1;
int timer2;

//pthread_mutex_t mutex;
pthread_mutex_t writert;
pthread_mutex_t readert;
pthread_mutex_t sharedData;


int main(int argc, char const *argv[])
{
   int i = 0;
   r = 0;
   w = 0;
   timer1 = 0;
   timer2 = 0;

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
      timer1 = atoi(argv[3]);
      timer2 = atoi(argv[4]);
      printf("r = %d, w = %d, t1 = %d, t2 = %d\n", r, w, timer1, timer2);

      //initialise Data
      data.readercount = 0;
      data.writercount = 0;
      data.writerDataIndex = 0;
      data.dataBufferIndex = 0;
      data.readersRun = r;

      //initialising data buffer to zero
      for(int i = 0; i < 20; i++)
      {
         data.dataBuffer[i] = 0;
      }

      // reading from shared_data into array
      data.fp = fopen("shared_data", "r");
      if (data.fp == NULL)
      {
         printf("Error: Could not open file\n");
      }
      else
      {
         while (feof(data.fp) == 0)
         {
            fscanf(data.fp, "%d,", &data.sharedData[i]);
            i++;
         }
         if(ferror(data.fp))
         {
            perror("Error while reading.");
         }
         fclose(data.fp);
      }
      //opening file writer to write to the file sim_out
      data.fp = fopen("sim_out", "w");

      //pthread_mutex_init(&mutex, NULL);
      pthread_mutex_init(&writert, NULL);
      pthread_mutex_init(&readert, NULL);
      pthread_mutex_init(&sharedData, NULL);

      produceThreads();
   }
   fclose(data.fp); //closing file pointer

   // final print of the reader and writer counts
   printf("\nreadcount: %d\nwritecount: %d\n", data.readercount, data.writercount);

   //destory threads;
   //pthread_mutex_destroy(&mutex);
   pthread_mutex_destroy(&writert);
   pthread_mutex_destroy(&readert);
   pthread_mutex_destroy(&sharedData);

   return 0;
}

void produceThreads()
{
   //create threads for readers
   pthread_t rthreads[r];
   //create threads for writers
   pthread_t wthreads[w];

   for (int i = 0; i < r; i++)
   {
      pthread_create(&rthreads[i], NULL, &reader, NULL);
   }
   for (int i = 0; i < w; i++)
   {
      pthread_create(&wthreads[i], NULL, &writer, NULL);
   }

   //joining
   for (int i = 0; i < r; i++)
   {
      pthread_join(rthreads[i], NULL);
   }
   for (int i = 0; i < w; i++)
   {
      pthread_join(wthreads[i], NULL);
   }
}

void* reader(void* nothing)
{
   int readSize = 0; //amount of data read
   int readData = 0; //read data cant be zero so this can be intiialised

   printf("\nI am reader: %d", (int)pthread_self());

   //pthread_mutex_lock(&mutex);

   while(readSize != 100) // while each reader process has not read 100 times
   {
      pthread_mutex_lock(&readert); //lock reader 0
         data.readercount++;
         if(data.readercount == 1)
         {
            pthread_mutex_lock(&writert); //lock writer 0
         }
      pthread_mutex_unlock(&readert); //unlock reader 1

         //reading is preformed
         if(data.dataBufferIndex == 20) //if the writers have filled the data buffer
         {
            if (data.readersRun == r) // if the current reader processes running equal the total readers created
            {
               for(int i = 0; i < 20; i++) // then read from data buffer
               {
                  readData = data.dataBuffer[i];
                  printf("\nReader: %d    Read Data: %d\n", (int)pthread_self(), readData); //print read data
                  readSize++;
                  sleep(timer1);
               }
               data.readersRun--; // decrease running readers since reading has finished
            }
         }
         //end reading

         //reseting buffer
         if(data.readersRun == 0)
         {
            for (int i = 0; i < 20; i++)
            {
               data.dataBuffer[i] = 0; //reset buffer to 0;
            }
            data.readersRun = r;// sets running readers back to enter value for the next set of 20 elements
            data.dataBufferIndex = 0; //setting index back to 0 so the writer starts from element 0 when writing to buffer
         }

      pthread_mutex_lock(&readert); //locks reader 0
         data.readercount--;
         if(data.readercount == 0)
         {
            pthread_mutex_unlock(&writert); //unlock writer 1
         }
         pthread_mutex_unlock(&readert); //unlock reader 1
   }
   printf("\nreadSize: %d\n", readSize);

   pthread_mutex_lock(&sharedData);
      fprintf(data.fp, "\nReader-%d has finished reading %d pieces of data from the data_buffer\n", (int)pthread_self(), readSize); //printing out to sim_out
   pthread_mutex_unlock(&sharedData);

   //pthread_mutex_unlock(&mutex);

   pthread_exit(NULL); //terminate thread
}

void* writer(void* nothing)
{
   int writeSize = 0; //amount of data written
   int writeData = 0; //read data cant be zero so this can be intiialised
   printf("\nI am writer: %d", (int) (int)pthread_self());
   //pthread_mutex_lock(&mutex);

   pthread_mutex_lock(&writert);
   while(data.writerDataIndex != 100)
   {
      pthread_mutex_unlock(&writert);
            pthread_mutex_lock(&writert);
               if(data.dataBufferIndex < 20) //buffer is no full
               {
                  writeData = data.sharedData[data.writerDataIndex]; //get data from shared data
                  data.dataBuffer[data.dataBufferIndex] = writeData; //write the recieved data to the data buffer
                  printf("\nWriter: %d    Written Data: %d    DataBuffer[%d]: %d \n", (int)pthread_self(), writeData, data.dataBufferIndex, data.dataBuffer[data.dataBufferIndex]);   //print write data
                  data.writercount++; //increment counters
                  data.writerDataIndex++;
                  data.dataBufferIndex++;
                  writeSize++;
                  sleep(timer2);
               }
            pthread_mutex_unlock(&writert);
   }
   pthread_mutex_lock(&sharedData);
      fprintf(data.fp, "\nWriter-%d has finished writing %d pieces of data to the data_buffer\n", (int)pthread_self(), writeSize); //print out to sim_out
   pthread_mutex_unlock(&sharedData);
   //pthread_mutex_unlock(&mutex);

   pthread_exit(NULL); //terminate thread
}
