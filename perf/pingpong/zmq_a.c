#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <zmq.h>
#include <sc/utils.h>

void _dealloc(void *data, void *hint)
{
  free(data);
}

int main(int argc, char *argv[])
{
  if (argc < 3) return EXIT_FAILURE;
  int M = atoi(argv[1]);
  int N = atoi(argv[2]);
  printf("M: %d, N: %d\n", M, N);

  void *ctx = zmq_init(1);
  void *b = zmq_socket(ctx, ZMQ_PAIR);
  zmq_connect(b, "tcp://localhost:4444");

  zmq_msg_t msg;
  int val[M];
  long long start_time = sc_time();

  int i;
  for (i=0; i<N; i++) {
    int *buf = (int *)malloc(M * sizeof(int));
    memset(val, i, M * sizeof(int));
    memcpy(buf, val, M * sizeof(int));
    zmq_msg_init_data(&msg, buf, M * sizeof(int), _dealloc, NULL);
    zmq_send(b, &msg, 0);
    zmq_msg_close(&msg);

    zmq_msg_init(&msg);
    zmq_recv(b, &msg, 0);
    memcpy(val, (int *)zmq_msg_data(&msg), zmq_msg_size(&msg));
    zmq_msg_close(&msg);
  }

  long long end_time = sc_time();

  printf("zmq_a: Time elapsed: %f sec\n", sc_time_diff(start_time, end_time));

  zmq_close(b);
  zmq_term(ctx);

  return EXIT_SUCCESS;
}
