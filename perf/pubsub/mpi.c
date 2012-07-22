#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpi.h>

#define TAG 100


int main(int argc, char *argv[])
{
  int rank, size;

  if (argc < 2) return EXIT_FAILURE;
  int N = atoi(argv[1]); // Number of iterations

  printf("N: %d\n", N);

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int val[N];
  double start_time = MPI_Wtime();

  MPI_Scatter(val, N, MPI_INT, val, N, MPI_INT, 0, MPI_COMM_WORLD); // 0 -> 1,2
  MPI_Gather(val, N, MPI_INT, val, N, MPI_INT, 0, MPI_COMM_WORLD); // 1,2 -> 0

  double end_time = MPI_Wtime();

  printf("%d: Time elapsed %f sec\n", rank, end_time - start_time);

  MPI_Finalize();

  return EXIT_SUCCESS;
}

