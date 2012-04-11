#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zmq.h>

#include "sc/primitives.h"


/**
 * \brief Helper function to deallocate send queue.
 *
 */
void _dealloc(void *data, void *hint)
{
  free(data);
}


int send_int(int val, role *r)
{
  int rc = 0;
  zmq_msg_t msg;

#ifdef __DEBUG__
  fprintf(stderr, " --> %s(%d) ", __FUNCTION__, val);
#endif

  int *buf = malloc(sizeof(int));
  memcpy(buf, &val, sizeof(int));

  zmq_msg_init_data(&msg, buf, sizeof(int), _dealloc, NULL);
  rc = zmq_send(r, &msg, 0);
  zmq_msg_close(&msg);
 
#ifdef __DEBUG__
  fprintf(stderr, ".\n");
#endif

  return rc;
}


int vsend_int(int val, int nr_of_roles, ...)
{
  int rc = 0;
  int i;
  role *r;
  va_list roles;
  session *s;

#ifdef __DEBUG__
  fprintf(stderr, " --> %s(%d)@%d ", __FUNCTION__, val, nr_of_roles);
#endif
  va_start(roles, nr_of_roles);
  for (i=0; i<nr_of_roles; i++) {
    r = va_arg(roles, role *);

    // Are all roles in the same session?
    if (i==0) s = r->s;
    assert(s == r->s);

#ifdef __DEBUG__
    fprintf(stderr, "   +");
#endif
    rc |= send_int(val, r);
  }
  va_end(roles);

#ifdef __DEBUG__
  fprintf(stderr, ".\n");
#endif

  return rc;
}


int recv_int(int *dst, role *r)
{
  int rc = 0;
  zmq_msg_t msg;

#ifdef __DEBUG__
  fprintf(stderr, " <-- %s() ", __FUNCTION__);
#endif

  zmq_msg_init(&msg);
  rc = zmq_recv(r, &msg, 0);
  assert(zmq_msg_size(&msg) == sizeof(int));
  memcpy(dst, (int *)zmq_msg_data(&msg), zmq_msg_size(&msg));
  zmq_msg_close(&msg);

#ifdef __DEBUG__
  fprintf(stderr, "[%d] .\n", *dst);
#endif

  return rc;
}