#include <stdio.h>
#include <string.h>
#include <mpi.h>

const int BUF = 64;

int main(int argc, char **argv) {
  char msg[BUF];
  int comm_sz;
  int rank;

  MPI_Init(NULL, NULL);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank > 0) {
    sprintf(msg, "Hello from %d of %d", rank, comm_sz);
    MPI_Send(msg, strlen(msg)+1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
  } else {
      printf("The master %d\n", rank);
      int i;
      for (i = 0; i < comm_sz; i++) {
        MPI_Recv(msg, BUF, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("%s\n", msg);
      }
    }
  MPI_Finalize();
  return 0;
}
