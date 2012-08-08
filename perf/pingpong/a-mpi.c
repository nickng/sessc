#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>

#include <sc-mpi.h>

int main(int argc, char *argv[])
{
  session *s;

  session_init(&argc, &argv, &s, "Pingpong_A.spr");

  if (argc < 3) return EXIT_FAILURE;
  int M = atoi(argv[1]);
  int N = atoi(argv[2]);
  printf("M: %d, N: %d\n", M, N);

  role *B = s->r(s, "B");

  int val[M];
  long long start_time = sc_time();

  int i;
  for (i=0; i<N; i++) {
    memset(val, i, M * sizeof(int));
    send_int_array(val, (size_t)M, B, 0);
    recv_int_array(val, M, B);
  }

  long long end_time = sc_time();

  printf("%s: Time elapsed: %f sec\n", s->name, sc_time_diff(start_time, end_time));

  session_dump(s);

  session_end(s);

  return EXIT_SUCCESS;
}
