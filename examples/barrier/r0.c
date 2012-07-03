#include <stdio.h>
#include <stdlib.h>

#include <sc.h>

int main(int argc, char *argv[])
{
  session *s;
  session_init(&argc, &argv, &s, "Protocol_R0.spr");
  session_dump(s);

  long long barrier_start = sc_time();
  barrier(s->r(s, "_Others"), "R0");
  long long barrier_end = sc_time();

  sc_print_version();
  printf("%s: Barrier time: %f sec\n", s->name, sc_time_diff(barrier_start, barrier_end));

  session_end(s);

  return EXIT_SUCCESS;
}
