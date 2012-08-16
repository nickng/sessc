#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zmq.h>
#include <sc/utils.h>


static void _dealloc(void *data, void *hint)
{
  free(data);
}

int main(int argc, char *argv[])
{
  if (argc < 2) return EXIT_FAILURE;
  int N = atoi(argv[1]);

  printf("N: %d\n", N);

  void *ctx = zmq_init(1);
  void *server = zmq_socket(ctx, ZMQ_REQ); // Server
  assert(server);

  int rc;
  rc = zmq_connect(server, "tcp://localhost:8889"); // Actively connect to subscribers
  assert(rc == 0);

  int *val = (int *)calloc(N, sizeof(int));
  zmq_msg_t msg;

  long long start_time = sc_time();

  // Send
  int *buf = (int *)calloc(N, sizeof(int));
  memcpy(buf, val, N * sizeof(int));
  zmq_msg_init_data(&msg, buf, N * sizeof(int), _dealloc, NULL);
  zmq_send(server, &msg, 0);
  zmq_msg_close(&msg);

  // Receive
  zmq_msg_init(&msg);
  zmq_recv(server, &msg, 0);
  memcpy(val, (int *)zmq_msg_data(&msg), zmq_msg_size(&msg));
  zmq_msg_close(&msg);

  long long end_time = sc_time();

  printf("%s: Time elapsed: %f sec\n", argv[0], sc_time_diff(start_time, end_time));

#ifdef __DEBUG__
  int i;
  printf("%s [ ", argv[0]);
  for (i=0; i<N; ++i) {
    printf("%d ", val[i]);
  }
  printf("]\n");
#endif

  free(val);
  zmq_close(server);
  zmq_term(ctx);

  return EXIT_SUCCESS;
}
