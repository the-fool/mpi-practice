#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include <stddef.h>

#define MAX_STRING 1024
#define MAX_VECTOR 40
#define MAX_LABEL 16

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
    printf("I am master process %d out of %d processes in the system\n", rank, comm_sz);
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
  int local_v[comm_sz];
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
    MPI_Recv(&msg, 1, MESSAGE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
    local_l++;
    local_v[rank]++;
    label[1]++;
    if (msg.dest == 0) {
      if ( strcmp(msg.string, "end") == 0) {
	printf("Process %d exitng\n", rank);
	break;
      }
      printf("Executing event %s in process %d.\n", label, rank);
    }  
    else if (status.MPI_SOURCE == 0) {
      printf("Message sent event %s from process %d to process %d: %s\n", 
	     label, rank, msg.dest, msg.string);
      mergeVectors(msg.vector, local_v, comm_sz);
      msg.lamport = local_l;
      MPI_Send(&msg, 1, MESSAGE, msg.dest, 0, MPI_COMM_WORLD); 
    }  
    else {
      printf("Message received event %s from process %d by process %d: %s\n", 
	     label, status.MPI_SOURCE, rank, msg.string);
      mergeVectors(msg.vector, local_v, comm_sz);
      local_l = (msg.lamport >= local_l ? msg.lamport : local_l);
    }
    strVector = vectorToString(local_v, comm_sz);
    printf("The Logical/Vector time of event %s at process %d is: %d / %s\n", 
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
  char * str = malloc(MAX_STRING); // who knows how many chars our time-stamps will require? 
  str[0] = '(';
  for (i = 0; i < comm_sz; i++) {
    n += sprintf(str + n + 1, "%d, ", v[i]);
  }
  sprintf(str + n -1, ")"); // terminating \0
  return str;
}
  
void master(int comm_sz) {
  Message msg;
  char buf[MAX_STRING + 12], string[MAX_STRING];
  char *directive; 
  const char delim[2]=" ";
  int dest, q;

  // initialize msg vector
  for (q = 0; q < comm_sz; q++)
    msg.vector[q] = 0;
  
  printf("Usage: (exec | send) pid1 [pid2 message]\n");
  printf("Enter 'end' to quit\n");

  while (1) {
    fgets(buf, MAX_STRING, stdin);
    char *p;

    // strip newline
    if ( (p = strchr(buf, '\n')) != NULL) *p = '\0';
    
    // subsequent strtok will destroy original string, so it's copied
    strcpy(string, buf);

    if ( (directive = strtok(buf, delim)) == NULL)
      continue; 
    
    if (strncmp(directive, "end", 3) == 0)
      break;
    
    if ( (p = strtok(NULL,delim)) == NULL) {
      printf("Invalid syntax\n");
      continue;
    }
    if ( (dest = atoi(p)) >= comm_sz || (dest < 1) ) { 
      printf("Invalid worker rank: %d\n", dest);
      continue;
    }
    if ( strncmp(directive, "send", 4) == 0 ) {
      if ( (p = strtok(NULL, delim)) == NULL) {
	printf("Invalid syntax\n");
	continue;
      }
      if ( (msg.dest = atoi(p)) >= comm_sz || (msg.dest < 1) ) {
	printf("Invalid worker rank: %d\n", msg.dest);
	continue;
      }
      p = strtok(NULL, delim);
      if (p != '\0')
	// get the untokenized message
        // via pointer offset
	strcpy(msg.string, string + (p - buf)); 
      else 
	strcpy(msg.string, "<NONE>");
    }
    else if ( strncmp(directive, "exec", 4) == 0 ) {
      msg.dest = 0;
      // arbitrary garbage message
      sprintf(msg.string, "hello rank: %d", dest);
    }
    else {
      printf("Unkown command: %s\n", directive);
      continue;
    }

    MPI_Send(&msg, 1, MESSAGE, dest, 0, MPI_COMM_WORLD);
  } // end while-loop
  
  // tell all processes to finalize
  sprintf(msg.string, "end");
  msg.dest = 0;
  for (dest = 1; dest < comm_sz; dest++) {
    MPI_Send(&msg, 1, MESSAGE, dest, 0, MPI_COMM_WORLD);
  }
}

