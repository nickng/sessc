#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>

#include <sc-mpi.h>

int main(int argc, char *argv[])
{
  session *s;

  session_init(&argc, &argv, &s, "Pingpong_B.spr");

  if (argc < 3) return EXIT_FAILURE;
  int M = atoi(argv[1]);
  int N = atoi(argv[2]);
  printf("M: %d, N: %d\n", M, N);

  role *A = s->r(s, "A");

  int val[M];
  long long start_time = sc_time();

  int i;
  for (i=0; i<N; i++) {
    recv_int_array(val, M, A);
    send_int_array(val, (size_t)M, A, 0);
  }

  long long end_time = sc_time();

  printf("%s: Time elapsed: %f sec\n", s->name, sc_time_diff(start_time, end_time));

  session_end(s);

  return EXIT_SUCCESS;
}

