#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include <math.h>

double TrapezoidSum(double, double, int, double);
double f(double);
void interact(int, int, double*, double*, int*);

int main(void) {
  int rank, comm_sz, n, local_n;
  double a, b, h, local_a, local_b;
  double local_area, total_area;
  int source;

  clock_t begin, end;

  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

  interact(rank, comm_sz, &a, &b, &n);

  h = (b - a) / n;
  local_n = n/comm_sz;

  local_a = a + rank*local_n*h;
  local_b = local_a + local_n * h;
  local_area = TrapezoidSum(local_a, local_b, local_n, h);

  if (rank != 0) {
    MPI_Send(&local_area, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
  } else {
    begin = clock();
    total_area = local_area;
    for (source = 1; source < comm_sz; source++) {
      MPI_Recv(&local_area, 1, MPI_DOUBLE, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      total_area += local_area;
    }
  }
 
  if (rank == 0) {
    printf("Trap count: %d.  Integral range: %f to %f \n", n, a, b);
    printf("Area estimate: %.15e\n", total_area);
    end = clock();
    printf("time taken: %.17e\n", (double)(end - begin)); 
  }
  MPI_Finalize();
  return 0;
}

double TrapezoidSum(double a, double b, int n, double base) {
  double estimate, x;
  int i;

  estimate = (f(a) + f(b))/2.0;
  for (i = 1; i <= n - 1; i++) {
    x = a + i*base;
    estimate += f(x);
  }
  estimate = estimate * base;
  return estimate;
}

double f(double x) {
  return (sin(x) + 5);
}

void interact(int rank, int comm_sz, double* ap, double* bp, int* np) {
  int send;

  if (rank == 0) {
    printf("Enter range of integral a, b, and number of trapezoids n\n");
    scanf("%lf %lf %d", ap, bp, np);
    for (send = 1; send < comm_sz; send++) {
      MPI_Send(ap, 1, MPI_DOUBLE, send, 0, MPI_COMM_WORLD);
      MPI_Send(bp, 1, MPI_DOUBLE, send, 0, MPI_COMM_WORLD);
      MPI_Send(np, 1, MPI_INT, send, 0, MPI_COMM_WORLD);
    }
  } else {
    MPI_Recv(ap, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(bp, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(np, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  }
}
