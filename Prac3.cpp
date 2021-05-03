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
#include <time.h>

void CSV_add(int,double,double,double,double);
FILE *fpt;

/** This is the master node function, describing the operations
    that the master will be doing */
void Master (int counter) {
 //! <h3>Local vars</h3>
 // The above outputs a heading to doxygen function entry
 int  j;             //! j: Loop counter
 char buff[BUFSIZE]; //! buff: Buffer for transferring message data
 MPI_Status stat;    //! stat: Status of the MPI application

 // Start of "Hello World" example..............................................
 //printf("0: We have %d processors\n", numprocs);
 for(j = 1; j < numprocs; j++) {
  //sprintf(buff, "Hello %d! ", j);
  MPI_Send(buff, BUFSIZE, MPI_CHAR, j, TAG, MPI_COMM_WORLD);
 }
 for(j = 1; j < numprocs; j++) {
  // This is blocking: normally one would use MPI_Iprobe, with MPI_ANY_SOURCE,
  // to check for messages, and only when there is a message, receive it
  // with MPI_Recv.  This would let the master receive messages from any
  // slave, instead of a specific one only.
  MPI_Recv(buff, BUFSIZE, MPI_CHAR, j, TAG, MPI_COMM_WORLD, &stat);
  //printf("0: %s\n", buff);
 }
 // End of "Hello World" example................................................

 // Read the input image
 if(!Input.Read("Data/fly.jpg")){
  //printf("Cannot read image\n");
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

 // Send Workers information they need-------------------------------------------
 //start timer
 struct timespec begin, middle1, middle2, middle3, end;
 clock_gettime(CLOCK_REALTIME, &begin);
  
 int split = Input.Height / (numprocs-1);	//How many rows each worker should get
 for(j = 1; j < numprocs; j++){
  int info[4]; // Row Offset, Number of rows (height), Row width, Image components

  // Allocate image data to each worker
  info[0] = split*(j-1);	// Row offset
  info[1] = info[0]+split; // Height (end)
  if(j == numprocs-1){
   info[1] = Input.Height; // Last worker processes the remainder
  }
  int step = info[1]-info[0]; //number of rows
  info[2] = Input.Width; // Row width
  info[3] = Input.Components; // Image components

  // Send info to worker
  MPI_Send(&info, 4, MPI_INT, j, ACK, MPI_COMM_WORLD);
  // -------------------------------------------------------

  // Split data and send to workers-------------------------------
  // Buffers to send partitioned image data
  int buffSize = step*info[2]*info[3];
  int inBuffer[buffSize]; // Rows*Width*Components
  int outBuffer[buffSize]; 

  // Fill input buffer

  int i = 0;
  
  //middle timer tells how long organising workers took
  clock_gettime(CLOCK_REALTIME, &middle1);

  for(int y = info[0]; y < info[1]; y++){ // Start at offset until end
   for(int x = 0; x < (Input.Width)*Input.Components; x++){ 
    inBuffer[i*Input.Width*Input.Components + x] = Input.Rows[y][x];
    outBuffer[i*Input.Width*Input.Components + x] = Input.Rows[y][x]; // Also fill the output buffer for values which are not processed
   }
   i++; // Track the start of our new buffer partitions, irrespective of offset
  }

  // Send work to worker, the partition he's responsible for in both input and output forms
  MPI_Send(&inBuffer, buffSize, MPI_INT, j, TAG, MPI_COMM_WORLD);
  MPI_Send(&outBuffer, buffSize, MPI_INT, j, TAG, MPI_COMM_WORLD);
 }
 
 //middle2 timer tells how long sending to workers took
 clock_gettime(CLOCK_REALTIME, &middle2);
 
 // Wait for workers to finish their tasks
 //printf("0: Waiting for workers...\n");
 // Data sent to workers-------------------------------------------

 // Receive data from workers--------------------------------------
 for(j = 1; j < numprocs; j++){
  // Receive info from worker to start filling Output image data
  int info[4]; // Offset, Height (end), width, components
  MPI_Recv(&info, 4, MPI_INT, j, ACK, MPI_COMM_WORLD, &stat);
  
  int offset = info[0];
  int end = info[1];
  int step = end - offset;
  int width = info[2];
  int components = info[3];

  int buffSize = step*width*components;
  int outBuffer[buffSize];
  // Fill outBuffer with image data
  MPI_Recv(&outBuffer, buffSize, MPI_INT, j, TAG, MPI_COMM_WORLD, &stat);
 
  //middle3 timer tells how long workers took to do work
  clock_gettime(CLOCK_REALTIME, &middle3);
  
  // Compile data  
  int i = 0;
  for(int y = offset; y < end; y++){ // Start at offset, end at offset+split
   for(int x = 0; x < width*components; x++){ 
    Output.Rows[y][x] = outBuffer[i*width*components + x];
   }
   i++;
  }
 }
 //printf("All workers done!\n");
 // Data received and compiled-------------------------------------
 
 //end timer also includes compiling data time
 clock_gettime(CLOCK_REALTIME, &end);
 
 //calc total time which includes compile
 long seconds = end.tv_sec - begin.tv_sec;
 long nanoseconds = end.tv_nsec - begin.tv_nsec;
 double elapsed = seconds + nanoseconds*1e-9;
 //printf("Total time measured: %.3f seconds.\n", elapsed);
 //calc organisation time
 seconds = middle1.tv_sec - begin.tv_sec;
 nanoseconds = middle1.tv_nsec - begin.tv_nsec;
 double organisationtime = seconds + nanoseconds*1e-9;
 //printf("Organisation time measured: %.3f seconds.\n", organisationtime);
 //calc sending to workers time
 seconds = middle2.tv_sec - middle1.tv_sec;
 nanoseconds = middle2.tv_nsec - middle1.tv_nsec;
 double sendingtime = seconds + nanoseconds*1e-9;
 //printf("Worker Sending time measured: %.3f seconds.\n", sendingtime);
 //calc workers doing job time
 seconds = middle3.tv_sec - middle2.tv_sec;
 nanoseconds = middle3.tv_nsec - middle2.tv_nsec;
 double jobtime = seconds + nanoseconds*1e-9;
 //printf("Workers working time measured: %.3f seconds.\n", jobtime);
 
 CSV_add(counter,organisationtime, sendingtime, jobtime, elapsed); //add the times to the csv file
 
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
 //sprintf(idstr, "Processor %d ", ID);
 //strncat(buff, idstr, BUFSIZE-1);
 //strncat(buff, "reporting for duty", BUFSIZE-1);

 // send to rank 0 (master):
 MPI_Send(buff, BUFSIZE, MPI_CHAR, 0, TAG, MPI_COMM_WORLD);
 // End of "Hello World" example................................................


 // Receive image info from master
 int info[4];
 MPI_Recv(&info, 4, MPI_INT, 0, ACK, MPI_COMM_WORLD, &stat);
 //printf("Worker %d here! I'll do rows %d to %d!\n", ID, info[0], info[1]);
 // End of receiving info block ------------------------------------------------
 int start = info[0];
 int end = info[1];
 int step = end-start;
 int width = info[2];
 int components = info[3];

 // Input and output buffers
 int buffSize = step*width*components;
 int inBuffer[buffSize]; // Rows*Width*Components
 int outBuffer[buffSize];

 // Receive data from master----------------------------------------------------
 MPI_Recv(&inBuffer, buffSize, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat);
 MPI_Recv(&outBuffer, buffSize, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat);
 //printf("Worker %d received data\n", ID);
 // Data Received--------------------------------------------------------------

 // Begin processing image using the median filter algorithm---------------------------------------
 int x,y;
 // Boundaries not processed
 for(y = 1; y < step-1; y++){
  for(x = 3; x < (width-1)*components; x++){
    
    //Store channel colour value of all neighbours
    int colours[9] = {
     inBuffer[(y+1)*width*components + x+3],
     inBuffer[(y+1)*width*components + x],
     inBuffer[(y+1)*width*components +x-3],
     inBuffer[(y)*width*components + x+3],
     inBuffer[(y)*width*components + x],
     inBuffer[(y)*width*components + x-3],
     inBuffer[(y-1)*width*components + x+3],
     inBuffer[(y-1)*width*components + x],
     inBuffer[(y-1)*width*components + x-3]
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
    outBuffer[y*width*components + x] = colours[4];
  }
 }
 // End of median filter -----------------------------------------------------------------------------

 // Send master info before sending processed image block
 MPI_Send(&info, 4, MPI_INT, 0, ACK, MPI_COMM_WORLD);
 // Send Processed partition back to master
 MPI_Send(&outBuffer, buffSize, MPI_INT, 0, TAG, MPI_COMM_WORLD);
 //printf("Worker %d done!\n", ID);
}
//------------------------------------------------------------------------------

void CSV_init(){
	fpt = fopen("MyFile.csv", "w+");
	fprintf(fpt,"ID, OrganisationTime, SendingToWorkersTime, WorkersWorkingTime, TotalTime\n"); //header rows for csv
}

void CSV_add(int counter,double organisationtime,double sendingtime,double jobtime,double elapsed){
	fprintf(fpt,"%d,%lf,%lf,%lf,%lf\n", counter,organisationtime, sendingtime, jobtime, elapsed); //adds all time data to csv
	
	
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
	 
	 //for testing purposes to make the graph the entire program is in a for loop and output to a CSV file
	 CSV_init();
   for(int counter = 0; counter<100;counter++){
		 if(myid == 0) Master(counter);
		 else          Slave (myid);
	 }
	 // MPI programs end with MPI_Finalize
	 MPI_Finalize();
	 fclose(fpt);
 printf("end of program ");
 return 0;
}
//------------------------------------------------------------------------------
