#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include <stddef.h>

#define MAX_STRING 1024
#define MAX_VECTOR 40
typedef struct message_s {
    char string[MAX_STRING];
    int vector[MAX_VECTOR];
    int dest;
  } Message;

MPI_Datatype MESSAGE;

void master(int comm_sz);
void slave(int rank, int comm_sz);
void create_struct_datatype();

int main(int argc, char **argv) 
{
  int rank, comm_sz;
  
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

  create_struct_datatype(&MESSAGE, comm_sz);
  
  if (rank != 0) {
    slave(rank, comm_sz); 
  }
  else {
    master(comm_sz);
  }

  MPI_Finalize();
  return 0;
}

void create_struct_datatype(MPI_Datatype *dt, int comm_sz) {
  int nitems = 3;
  int blocklengths[3] = { MAX_STRING, MAX_VECTOR, 1};
  MPI_Datatype types[3] = {MPI_CHAR, MPI_INT, MPI_INT};
  MPI_Aint offsets[3];

  offsets[0] = offsetof(Message, string);
  offsets[1] = offsetof(Message, vector);
  offsets[2] = offsetof(Message, dest);

  MPI_Type_create_struct(nitems, blocklengths, offsets, types, dt);
  MPI_Type_commit(dt);
}
  
void slave(int rank, int comm_sz) {
  Message msg;
  int local_v[comm_sz];
  MPI_Status status;
  int q;

  for (q = 0; q < comm_sz; q++) 
    local_v[q] = 0;
  
  while (1) {
    MPI_Recv(&msg, 1, MESSAGE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
    local_v[rank]++;
    if (msg.dest == 0) {
      if ( strcmp(msg.string, "end") == 0) {
	printf("Process %d exitng\n", rank);
	break;
      }
      printf("Rank: %d executing task.\n", rank);
    }  
    else if (status.MPI_SOURCE == 0) {
      printf("Rank: %d sending message to %d: %s\n", rank, msg.dest, msg.string);
      mergeVectors(msg.time_stamp, local_v);
      MPI_Send(&msg, 1, MESSAGE, msg.dest, 0, MPI_COMM_WORLD); 
    }  
    else {
      printf("Rank: %d received message from %d: %s\n", rank, status.MPI_SOURCE, msg.string);
      local_t = (msg.time_stamp >= local_t ? msg.time_stamp + 1 : local_t);
    }
    printf("Rank: %d local time: %d\n", rank, local_t);
  }
}

void mergeVectors(int* dest, int* src, int comm_sz) {
  int q;
  for (q = 0; q < comm_sz; q++) {
    dest[q] = (dest[q] >= src[q] ? dest[q] : src[q]);
  }
}
  
void master(int comm_sz) {
  Message msg;
  char buf[MAX_STRING + 12], string[MAX_STRING];
  char *directive; 
  const char delim[2]=" ";
  int dest;

  printf("Usage: (exec | send) pid1 [pid2 message]\n");
  printf("Enter 'end' to quit\n");

  while (1) {
    fgets(buf, MAX_STRING, stdin);
    char *p;
    if ( (p = strchr(buf, '\n')) != NULL) *p = '\0';

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
      strcpy(msg.string, string + (p - buf)); // get the untokenized message
    }
    else if ( strncmp(directive, "exec", 4) == 0 ) {
      msg.dest = 0;
      sprintf(msg.string, "hello rank: %d", dest);
    }
    else {
      printf("Unkown command: %s\n", directive);
      continue;
    }

    MPI_Send(&msg, 1, MESSAGE, dest, 0, MPI_COMM_WORLD);
  }
  
  sprintf(msg.string, "end");
  msg.dest = 0;
  for (dest = 1; dest < comm_sz; dest++) {
    MPI_Send(&msg, 1, MESSAGE, dest, 0, MPI_COMM_WORLD);
  }
}
