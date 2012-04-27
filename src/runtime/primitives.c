/**
 * \file
 * Session C runtime library (libsc)
 * communication primitives module.
 */

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


inline int send_int(int val, role *r)
{
  return send_int_array(&val, 1, r);
}


int send_int_array(const int arr[], size_t count, role *r)
{
  int rc = 0;
  zmq_msg_t msg;
  size_t size = sizeof(int) * count;

#ifdef __DEBUG__
  fprintf(stderr, " --> %s ", __FUNCTION__);
#endif

  int *buf = (int *)malloc(size);
  memcpy(buf, arr, size);

  zmq_msg_init_data(&msg, buf, size, _dealloc, NULL);
  switch (r->type) {
    case SESSION_ROLE_P2P:
      rc = zmq_send(r->p2p->ptr, &msg, 0);
      break;
    case SESSION_ROLE_GRP:
#ifdef __DEBUG__
      fprintf(stderr, "bcast -> %s(%d endpoints) ", r->grp->name, r->grp->nendpoint);
#endif

      rc = zmq_send(r->grp->out->ptr, &msg, 0);
      break;
    default:
        fprintf(stderr, "%s: Unknown endpoint type: %d\n", __FUNCTION__, r->type);
  }
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

#ifdef __DEBUG__
  fprintf(stderr, " --> %s(%d)@%d ", __FUNCTION__, val, nr_of_roles);
#endif
  va_start(roles, nr_of_roles);
  for (i=0; i<nr_of_roles; i++) {
    r = va_arg(roles, role *);

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


inline int recv_int(int *dst, role *r)
{
  size_t count;
  return recv_int_array(dst, &count, r);
}


int recv_int_array(int *arr, size_t *count, role *r)
{
  int rc = 0;
  zmq_msg_t msg;
  size_t size = -1;

#ifdef __DEBUG__
  fprintf(stderr, " <-- %s() ", __FUNCTION__);
#endif

  zmq_msg_init(&msg);
  switch (r->type) {
    case SESSION_ROLE_P2P:
      rc = zmq_recv(r->p2p->ptr, &msg, 0);
      break;
    case SESSION_ROLE_GRP:
#ifdef __DEBUG__
      fprintf(stderr, "bcast <- %s(%d endpoints) ", r->grp->name, r->grp->nendpoint);
#endif
      rc = zmq_recv(r->grp->in->ptr, &msg, 0);
      break;
    default:
        fprintf(stderr, "%s: Unknown endpoint type: %d\n", __FUNCTION__, r->type);
  }
  size = zmq_msg_size(&msg);
  if (*count * sizeof(int) >= size) {
    memcpy(arr, (int *)zmq_msg_data(&msg), size);
    if (size % sizeof(int) == 0) {
      *count = size / sizeof(int);
    }
  } else {
    memcpy(arr, (int *)zmq_msg_data(&msg), *count * sizeof(int));
    fprintf(stderr,
      "%s: Received data (%zu bytes) > memory size (%zu), data truncated\n",
      __FUNCTION__, size, *count * sizeof(int));
  }
  zmq_msg_close(&msg);

#ifdef __DEBUG__
  fprintf(stderr, "[%d ...] .\n", *arr);
#endif

  return rc;
}
