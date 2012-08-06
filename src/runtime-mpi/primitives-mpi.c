/**
 * \file
 * Session C runtime library MPI backend (libsc-mpi)
 * communication primitives module.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <mpi.h>

#include "sc/primitives-mpi.h"
#include "sc/types-mpi.h"

extern sc_req_tbl_t *sc_req_tbl;


static void sc_manage_request(int rank, void *ptr, MPI_Request req)
{
  if (ptr == NULL) {
    // This is a query.
    // Attempt to Wait and finish all message transmission

    MPI_Status *stats = (MPI_Status *)calloc(sizeof(MPI_Status), sc_req_tbl[rank].nsend);
    MPI_Waitall(sc_req_tbl[rank].nsend, sc_req_tbl[rank].send_reqs, stats);
    int i;
    for (i=0; i<sc_req_tbl[rank].nsend; ++i) {
      free(sc_req_tbl[rank].send_bufs[i]);
    }
    free(sc_req_tbl[rank].send_bufs);
    free(sc_req_tbl[rank].send_reqs);
    free(stats);
  } else {
    // This is a send request
    // Record message transmission

    if (0 == sc_req_tbl[rank].nsend) {
      sc_req_tbl[rank].nsend = 1;
      sc_req_tbl[rank].send_reqs = (MPI_Request *)malloc(sizeof(MPI_Request));
      sc_req_tbl[rank].send_reqs[0] = req;
      sc_req_tbl[rank].send_bufs[0] = ptr;
    } else {
      sc_req_tbl[rank].nsend++;
      sc_req_tbl[rank].send_reqs = (MPI_Request *)realloc(sc_req_tbl[rank].send_reqs, sizeof(MPI_Request) * sc_req_tbl[rank].nsend);
      sc_req_tbl[rank].send_reqs[sc_req_tbl[rank].nsend - 1] = req;
      sc_req_tbl[rank].send_bufs = (void **)realloc(sc_req_tbl[rank].send_bufs, sizeof(MPI_Request) * sc_req_tbl[rank].nsend);
      sc_req_tbl[rank].send_bufs[sc_req_tbl[rank].nsend - 1] = ptr;
    }
  }
}


inline int send_int(int val, role *r, int label)
{
  int rc;
  MPI_Request req;
  int *buf = (int *)malloc(sizeof(int));
  memcpy(buf, &val, sizeof(int));
  rc = MPI_Isend(buf, 1, MPI_INT, r->p2p->rank, label, r->p2p->comm, &req);
  sc_manage_request(r->p2p->rank, (void *)buf, req);
  return rc;
}


inline int send_int_array(const int arr[], size_t count, role *r, int label)
{
  int rc;
  MPI_Request req;
  int *buf = (int *)malloc(sizeof(int) * count);
  memcpy(buf, arr, sizeof(int) * count);
  rc = MPI_Isend(buf, count, MPI_INT, r->p2p->rank, label, r->p2p->comm, &req);
  sc_manage_request(r->p2p->rank, (void *)buf, req);
  return rc;
}

inline int recv_int(int *val, role *r)
{
  int rc;
  MPI_Status stat;
  rc = MPI_Recv(val, 1, MPI_INT, r->p2p->rank, 0, r->p2p->comm, &stat);
  sc_manage_request(r->p2p->rank, NULL, NULL);
  return rc;
}

inline int recv_int_array(int *arr, size_t count, role *r)
{
  int rc;
  MPI_Status stat;
  rc = MPI_Recv(arr, count, MPI_INT, r->p2p->rank, 0, r->p2p->comm, &stat);
  sc_manage_request(r->p2p->rank, NULL, NULL);
  return rc;
}
