#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpi.h>

#include <sc/utils.h>
#include "common.h"

#define TAG 100

int main(int argc, char *argv[])
{
  int rank, size;
  MPI_Request req;
  MPI_Status status;

  if (argc < 3) return EXIT_FAILURE;
  int M = atoi(argv[1]);
  int N = atoi(argv[2]);

  printf("M: %d, N: %d\n", M, N);

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int val[M];
  double start_time = MPI_Wtime();
  
  int i;
  for (i=0; i<N; i++) {
    if (rank == 0) {
      memset(val, i,  M * sizeof(int));
      MPI_Isend(val, M, MPI_INT, 1, TAG, MPI_COMM_WORLD, &req);
      MPI_Recv(val, M, MPI_INT, 1, TAG, MPI_COMM_WORLD, &status);
    } else if (rank == 1) {
      MPI_Recv(val, M, MPI_INT, 0, TAG, MPI_COMM_WORLD, &status);
      MPI_Isend(val, M, MPI_INT, 0, TAG, MPI_COMM_WORLD, &req);
    }
    MPI_Wait(&req, &status);
  }

  double end_time = MPI_Wtime();

  printf("%d: Time elapsed: %f sec\n", rank, end_time - start_time);

  MPI_Finalize();

  return EXIT_SUCCESS;
}
