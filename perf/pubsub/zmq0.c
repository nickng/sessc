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
  void *pub = zmq_socket(ctx, ZMQ_PUB); // Output channel of 0
  assert(pub);
  void *sub = zmq_socket(ctx, ZMQ_SUB); // Input channel of 0
  assert(sub);

  int rc;
  rc = zmq_bind(sub, "tcp://*:8887"); // Waits for publishers
  assert(rc == 0);

  rc = zmq_connect(pub, "tcp://localhost:8888"); // Actively connect to subscribers
  assert(rc == 0);

  zmq_setsockopt(sub, ZMQ_SUBSCRIBE, "", 0);

  int i;
  int *val = (int *)calloc(N, sizeof(int));
  zmq_msg_t msg;

  printf("[ ");
  for (i=0; i<N; ++i) {
    val[i] = i;
    printf("%d ", val[i]);
  }
  printf("]\n");

  long long start_time = sc_time();

  // Send
  int *buf = (int *)calloc(N, sizeof(int));
  memcpy(buf, val, N * sizeof(int));
  zmq_msg_init_data(&msg, buf, N * sizeof(int), _dealloc, NULL);
  zmq_send(pub, &msg, 0);
  zmq_msg_close(&msg);

  // Receive
  zmq_msg_init(&msg);
  zmq_recv(sub, &msg, 0);
  memcpy(val, (int *)zmq_msg_data(&msg), zmq_msg_size(&msg));
  zmq_msg_close(&msg);

  long long end_time = sc_time();

  printf("%s: Time elapsed: %f sec\n", argv[0], sc_time_diff(start_time, end_time));

  printf("%s [ ", argv[0]);
  for (i=0; i<N; ++i) {
    printf("%d ", val[i]);
  }
  printf("]\n");

  free(val);
  zmq_close(sub);
  zmq_close(pub);
  zmq_term(ctx);

  return EXIT_SUCCESS;
}
