//==============================================================================
// Copyright (C) John-Philip Taylor
// tyljoh010@myuct.ac.za
//
// This file is part of the EEE4120F Course
//
// This file is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>
//
// This is an adaptition of The "Hello World" example avaiable from
// https://en.wikipedia.org/wiki/Message_Passing_Interface#Example_program
//==============================================================================


/** \mainpage Prac3 Main Page
 *
 * \section intro_sec Introduction
 *
 * The purpose of Prac3 is to learn some basics of MPI coding.
 *
 * Look under the Files tab above to see documentation for particular files
 * in this project that have Doxygen comments.
 */



//---------- STUDENT NUMBERS --------------------------------------------------
//
// PNSSHA003 MLLDEC001
//
//-----------------------------------------------------------------------------

/* Note that Doxygen comments are used in this file. */
/** \file Prac3
 *  Prac3 - MPI Main Module
 *  The purpose of this prac is to get a basic introduction to using
 *  the MPI libraries for prallel or cluster-based programming.
 */

// Includes needed for the program
#include "Prac3.h"

/** This is the master node function, describing the operations
    that the master will be doing */
void Master () {
 //! <h3>Local vars</h3>
 // The above outputs a heading to doxygen function entry
 int  j;             //! j: Loop counter
 char buff[BUFSIZE]; //! buff: Buffer for transferring message data
 MPI_Status stat;    //! stat: Status of the MPI application

 // Start of "Hello World" example..............................................
 printf("0: We have %d processors\n", numprocs);
 for(j = 1; j < numprocs; j++) {
  sprintf(buff, "Hello %d! ", j);
  MPI_Send(buff, BUFSIZE, MPI_CHAR, j, TAG, MPI_COMM_WORLD);
 }
 for(j = 1; j < numprocs; j++) {
  // This is blocking: normally one would use MPI_Iprobe, with MPI_ANY_SOURCE,
  // to check for messages, and only when there is a message, receive it
  // with MPI_Recv.  This would let the master receive messages from any
  // slave, instead of a specific one only.
  MPI_Recv(buff, BUFSIZE, MPI_CHAR, j, TAG, MPI_COMM_WORLD, &stat);
  printf("0: %s\n", buff);
 }
 // End of "Hello World" example................................................

 // Read the input image
 if(!Input.Read("Data/fly.jpg")){
  printf("Cannot read image\n");
  return;
 }

 // Allocated RAM for the output image
 if(!Output.Allocate(Input.Width, Input.Height, Input.Components)) return;

 /* //This is example code of how to copy image files ----------------------------
 printf("Start of example code...\n");
 for(j = 0; j < 10; j++){
  tic();
  int x, y;
  for(y = 0; y < Input.Height; y++){
   for(x = 0; x < Input.Width*Input.Components; x++){
    Output.Rows[y][x] = Input.Rows[y][x];
   }
  }
  printf("Time = %lg ms\n", (double)toc()/1e-3);
 }
 printf("End of example code...\n\n");
 // End of example -------------------------------------------------------------*/

 JSAMPLE*** Partition; 
 JSAMPLE*** OutPartition; // Will use this for transfering partitions of our rows

 // Send Workers information they need-------------------------------------------
 int info[4]; // Row Offset, Number of rows (height), Row width, Image components
 int split = Input.Height / (numprocs-1);	//How many rows each worker should get
 for(j = 1; j < numprocs; j++){

  // Allocate image data to each worker
  info[0] = split*(j-1);	// Row offset
  info[1] = info[0]+split; // Number of rows
  if(j == numprocs-1){
   info[1] += Input.Height % (info[1]); // Last worker also processes the remainder
  }
  info[2] = Input.Width; // Row width
  info[3] = Input.Components; // Image components

  // Send info to worker
  MPI_Send(info, BUFSIZE, MPI_BYTE, j, ACK, MPI_COMM_WORLD);

 

  Partition = new JSAMPLE**[info[1]];	//New pointer array to process rows
  OutPartition = new JSAMPLE**[info[1]];

  for(int i = 0; i < info[1]; i++){
   Partition[i] = &Input.Rows[info[0]+i];	// Partition points to an array of rows
   OutPartition[i] = &Output.Rows[info[0]+i];
  }

  // Send work to worker, the partition he's responsible for in both input and output forms
  MPI_Send(Partition, BUFSIZE, MPI_BYTE, j, TAG, MPI_COMM_WORLD);
  MPI_Send(OutPartition, BUFSIZE, MPI_BYTE, j, TAG, MPI_COMM_WORLD);
 }

 // Wait for workers to finish their tasks
 printf("0: Waiting for workers...");
 for(j = 1; j < numprocs; j++){
  MPI_Recv(OutPartition, BUFSIZE, MPI_BYTE, j, TAG, MPI_COMM_WORLD, &stat);
 }
 printf("All workers done!");
 

 /*// Median filter algorithm implementation over whole image------------------------
 printf("Applying median filter...\n");
 int x,y,c;
 // Boundaries not processed
 for(y = 1; y < Input.Height-1; y++){
  for(x = 1; x < (Input.Width-1)*Input.Components; x++){

    //Store channel colour value of all neighbours
    int colours[9] = {
     Input.Rows[y+1][x+3],
     Input.Rows[y+1][x],
     Input.Rows[y+1][x-3],
     Input.Rows[y][x+3],
     Input.Rows[y][x],
     Input.Rows[y][x-3],
     Input.Rows[y-1][x+3],
     Input.Rows[y-1][x],
     Input.Rows[y-1][x-3]
    };

    // Sort array - insertion sort is fine, this is a small array - basic insertion sort algorithm
    int k;
    for(k=1; k < 9; k++){
     int val = colours[k];
     int pos = k;

     while(pos > 0 && colours[pos-1] > val){
      colours[pos] = colours[pos-1];
      pos = pos-1;
     }

     colours[pos] = val;
    }
    // End of sort

    // Median will be colours[4]
    Output.Rows[y][x] = colours[4];
  }
 }
 // End of median filter algorithm---------------------------------------------*/

 // Write the output image
 if(!Output.Write("Data/Output.jpg")){
  printf("Cannot write image\n");
  return;
 }
 //! <h3>Output</h3> The file Output.jpg will be created on success to save
 //! the processed output.

 delete[] Partition;
 delete[] OutPartition;
}
//------------------------------------------------------------------------------

/** This is the Slave function, the workers of this MPI application. */
void Slave(int ID){
 // Start of "Hello World" example..............................................
 char idstr[32];
 char buff [BUFSIZE];

 MPI_Status stat;

 // receive from rank 0 (master):
 // This is a blocking receive, which is typical for slaves.
 MPI_Recv(buff, BUFSIZE, MPI_CHAR, 0, TAG, MPI_COMM_WORLD, &stat);
 sprintf(idstr, "Processor %d ", ID);
 strncat(buff, idstr, BUFSIZE-1);
 strncat(buff, "reporting for duty", BUFSIZE-1);

 // send to rank 0 (master):
 MPI_Send(buff, BUFSIZE, MPI_CHAR, 0, TAG, MPI_COMM_WORLD);
 // End of "Hello World" example................................................


 // Receive image info from master
 int info[4];
 MPI_Recv(info, BUFSIZE, MPI_BYTE, 0, ACK, MPI_COMM_WORLD, &stat);
 printf("Worker %d here! I'll do rows %d to %d!\n", ID, info[0], info[1]);
 // End of receiving info block ------------------------------------------------

 int start = info[0];
 int step = info[1];
 int end = start+step;
 int width = info[2];
 int components = info[3];

 // Input and output partitions
 JSAMPLE*** Partition; 
 JSAMPLE*** OutPartition;

 // Receive data from master----------------------------------------------------
 MPI_Recv(Partition, BUFSIZE, MPI_BYTE, 0, TAG, MPI_COMM_WORLD, &stat);
 MPI_Recv(OutPartition, BUFSIZE, MPI_BYTE, 0, TAG, MPI_COMM_WORLD, &stat);
 printf("Worker %d received data\n", ID);
 // Data Received--------------------------------------------------------------

 // Begin processing image using the median filter algorithm---------------------------------------
 int x,y,c;
 // Boundaries not processed
 for(y = 1; y < step-1; y++){
  for(x = 1; x < (width-1)*components; x++){
    
    //Store channel colour value of all neighbours
    int colours[9] = {
     Partition[y+1][x+3],
     Partition[y+1][x],
     Partition[y+1][x-3],
     Partition[y][x+3],
     Partition[y][x],
     Partition[y][x-3],
     Partition[y-1][x+3],
     Partition[y-1][x],
     Partition[y-1][x-3]
    };


    // Sort array - insertion sort is fine, this is a small array - basic insertion sort algorithm
    int k;
    for(k=1; k < 9; k++){
     int val = colours[k];
     int pos = k;

     while(pos > 0 && colours[pos-1] > val){
      colours[pos] = colours[pos-1];
      pos = pos-1;
     }

     colours[pos] = val;
    }
    // End of sort

    // Median will be colours[4]
    OutPartition[y][x] = colours[4];
  }
 }
 // End of median filter -----------------------------------------------------------------------------

 // Send Processed partition back to master
 MPI_Send(OutPartition, BUFSIZE, MPI_BYTE, 0, TAG, MPI_COMM_WORLD);
 printf("Worker %d done!\n", ID);
}
//------------------------------------------------------------------------------

/** This is the entry point to the program. */
int main(int argc, char** argv){
 int myid;

 // MPI programs start with MPI_Init
 MPI_Init(&argc, &argv);

 // find out how big the world is
 MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

 // and this processes' rank is
 MPI_Comm_rank(MPI_COMM_WORLD, &myid);

 // At this point, all programs are running equivalently, the rank
 // distinguishes the roles of the programs, with
 // rank 0 often used as the "master".
 if(myid == 0) Master();
 else          Slave (myid);

 // MPI programs end with MPI_Finalize
 MPI_Finalize();
 return 0;
}
//------------------------------------------------------------------------------
