#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>

#define MAX_STRING 1024 
#define MAX_VECTOR 40  // no. of total processes

typedef struct message_s {
  char string[MAX_STRING];
  int vector[MAX_VECTOR];
  int lamport;
  int dest;
  } Message;

MPI_Datatype MESSAGE;

void master(int comm_sz);
void slave(int rank, int comm_sz);
void create_struct_datatype();
void mergeVectors();
char* vectorToString();
 
int main(int argc, char **argv) 
{
  int rank, comm_sz;
  
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

  create_struct_datatype(&MESSAGE, comm_sz);
  
  if (rank != 0) {
    printf("I am process %d out of %d processes in the system\n", rank, comm_sz);
    slave(rank, comm_sz); 
  }
 
  else {
    sleep(1);
    printf("I am master process %d out of %d processes in the system\n\n", rank, comm_sz);
    master(comm_sz);
  }

  MPI_Finalize();
  return 0;
}

void create_struct_datatype(MPI_Datatype *dt, int comm_sz) {
  // create our special struct datatype for message-passing
  int nitems = 4;
  int blocklengths[4] = { MAX_STRING, MAX_VECTOR, 1, 1};
  MPI_Datatype types[4] = { MPI_CHAR, MPI_INT, MPI_INT, MPI_INT};
  MPI_Aint offsets[4];

  offsets[0] = offsetof(Message, string);
  offsets[1] = offsetof(Message, vector);
  offsets[2] = offsetof(Message, lamport);
  offsets[3] = offsetof(Message, dest);

  MPI_Type_create_struct(nitems, blocklengths, offsets, types, dt);
  MPI_Type_commit(dt);
}
  
void slave(int rank, int comm_sz) {
  Message msg;
  int local_v[MAX_VECTOR];
  int local_l;
  MPI_Status status;
  
  int q; // disposable index
  char * strVector; // pointer for string representation of vector
  char label[3]; // storage for local event label 
  
  // initialize vector
  for (q = 0; q < comm_sz; q++) 
    local_v[q] = 0;
  // initialize lamport
  local_l = 0;
  // initialize label
  label[0] = '0' + rank - 1;
  label[1] = '@'; // == 'A' - 1
  label[2] = '\0';

   while (1) {
    MPI_Recv(&msg, 1, MESSAGE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    sleep(1);
    // On finalize, MPI_TAG == 1
    if (status.MPI_TAG == 1) {
      strVector = vectorToString(local_v, comm_sz);
      sprintf(msg.string, "Process %d report: Event %s - Logical: %d - Vector: %s", 
	      rank, label, local_l, strVector);
      free(strVector);
      MPI_Send(&msg, 1, MESSAGE, 0, 0, MPI_COMM_WORLD);
      break;
    }

    local_l++;
    local_v[rank]++;
    label[1]++;

    if (status.MPI_SOURCE == 0) {
      if (msg.dest == 0) {
	printf("Executing event %s in process %d.\n", label, rank);
      }
      else {
	printf("Message sent event %s from process %d to process %d: %s\n", 
	       label, rank, msg.dest, msg.string);
	for (q = 0; q < comm_sz; q++) {
	  msg.vector[q] = local_v[q];
	}
	msg.lamport = local_l;
	MPI_Send(&msg, 1, MESSAGE, msg.dest, 0, MPI_COMM_WORLD); 
      }
      sprintf(msg.string, "receievd");
      MPI_Send(&msg, 1, MESSAGE, 0, 0, MPI_COMM_WORLD);     
    } else {
      printf("Message received event %s from process %d by process %d: %s\n", 
	     label, status.MPI_SOURCE, rank, msg.string);
      mergeVectors(msg.vector, local_v, comm_sz);
      local_l = (msg.lamport >= (local_l - 1)? msg.lamport + 1 : local_l);
    }

    // print status to stdout
    strVector = vectorToString(local_v, comm_sz);
    printf("The Logical/Vector time of event %s at process %d is: %d / %s\n\n", 
	   label, rank, local_l, strVector);
    free(strVector);
  }
}

void mergeVectors(int* msg, int* loc, int comm_sz) {
  int q;
  for (q = 0; q < comm_sz; q++) {
    if (msg[q] < loc[q]) msg[q] = loc[q];
    else loc[q] = msg[q];
  }
}

char * vectorToString(int *v, int comm_sz) {
  int i, n = 0;
  char * str = malloc(MAX_STRING);  
  str[0] = '(';
  for (i = 1; i < comm_sz; i++) 
    // begin at i = 1 in order to ignore Master process at 0
    {
      n += sprintf(str + n + 1, "%d, ", v[i]);
    }
  sprintf(str + n -1, ")");
  return str;
}
  
void master(int comm_sz) {
  Message msg;
  MPI_Status status;
  int dest, q;
  // a series of tuples denoting destination of message, and route for response
  int directives[8][2] = {1,2, 3,0, 4,0, 2,1, 3,2, 3,4, 2,0, 4,0};
  
  // initialize msg vector
  for (q = 0; q < comm_sz; q++)
    msg.vector[q] = 0;

  // process the script
  for (q=0; q < 8; q++) {
    sprintf(msg.string, "Master event %d", q);
    dest = directives[q][0];
    msg.dest = directives[q][1];
    MPI_Send(&msg, 1, MESSAGE, dest, 0, MPI_COMM_WORLD);
    MPI_Recv(&msg, 1, MESSAGE, dest, 0, MPI_COMM_WORLD, &status);
  }

  sleep(1);

  // signal all processes to finalize
  msg.dest = 0;
  for (dest = 1; dest < comm_sz; dest++) {
    MPI_Send(&msg, 1, MESSAGE, dest, 1, MPI_COMM_WORLD);
    MPI_Recv(&msg, 1, MESSAGE, dest, 0, MPI_COMM_WORLD, &status);
    printf("%s\n", msg.string);
  }
}

