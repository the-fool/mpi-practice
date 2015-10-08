#include <mpi.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

int main ( int argc, char **argv )
{
  int rank, comm_sz;
  char hostname[256];
  char msg[128];

  MPI_Init( &argc, &argv );
  MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  gethostname(hostname, 255);

  if (rank != 0) {
    printf("HELLO!\n");
    sprintf(msg, "I am rank %d", rank);
    MPI_Send(msg, strlen(msg)+1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);    
  } else {
    printf("Hey there, I am number %d on host %s\n", rank, hostname);
    int i;
    for(i = 1; i < comm_sz; i++) {
      MPI_Recv(msg, 128, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      printf("%s\n", msg); 
    } 
  }
  MPI_Finalize();

  return 0;
}
