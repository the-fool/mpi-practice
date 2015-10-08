#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include <stddef.h>

#define MAX_STRING 1024
typedef struct message_s {
    char string[MAX_STRING];
    int time_stamp;
    int dest;
  } Message;

void interact();
void slave();
void create_struct_datatype();

int main(int argc, char **argv) 
{
  int rank, comm_sz, local_t;
  MPI_Datatype MESSAGE;
    
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

  create_struct_datatype(&MESSAGE);

  local_t = 0;
  
  if (rank != 0) {
    Message recv;
    MPI_Recv(&recv, 1, MESSAGE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    printf("I am rank %d, I got the string %s\n", rank, recv.string); 
  }
  else {
    int q;
    for (q = 1; q < comm_sz; q++) {
      Message send;
      sprintf(send.string, "Master to rank: %d", q);
      MPI_Send(&send, 1, MESSAGE, q, 0, MPI_COMM_WORLD);
    }
  }

  MPI_Finalize();
  return 0;
}

void create_struct_datatype(MPI_Datatype *dt) {
  int nitems = 3;
  int blocklengths[3] = { MAX_STRING, 1, 1};
  MPI_Datatype types[3] = {MPI_CHAR, MPI_INT, MPI_INT};
  MPI_Aint offsets[3];

  offsets[0] = offsetof(Message, string);
  offsets[1] = offsetof(Message, time_stamp);
  offsets[2] = offsetof(Message, dest);

  MPI_Type_create_struct(nitems, blocklengths, offsets, types, dt);
  MPI_Type_commit(dt);
}
  
    
