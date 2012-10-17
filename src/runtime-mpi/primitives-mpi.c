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

#include "sc/errors.h"
#include "sc/debug.h"
#include "sc/primitives-mpi.h"
#include "sc/types-mpi.h"

extern sc_req_tbl_t *sc_req_tbl;


static void sc_manage_request(int rank, void *ptr, MPI_Request req)
{
  sc_debug("INFO", "%s entry", __FUNCTION__);
  if (ptr == NULL) {
    // This is a query.
    // Attempt to Wait and finish all message transmission
    sc_debug("REQ.QUERY", "{ rank:%d nsend:%d }", rank, sc_req_tbl[rank].nsend);

    if (sc_req_tbl[rank].nsend > 0) {
      MPI_Waitall(sc_req_tbl[rank].nsend, sc_req_tbl[rank].send_reqs, MPI_STATUSES_IGNORE);
      int i;
      for (i=0; i<sc_req_tbl[rank].nsend; ++i) {
        free(sc_req_tbl[rank].send_bufs[i]);
      }
      free(sc_req_tbl[rank].send_bufs);
      free(sc_req_tbl[rank].send_reqs);
      sc_req_tbl[rank].nsend = 0;
    }
  } else {
    // This is a send request
    // Record message transmission
    sc_debug("REQ.QUEUE", "{ rank:%d nsend:%d->%d }", rank, sc_req_tbl[rank].nsend, sc_req_tbl[rank].nsend+1);

    if (0 == sc_req_tbl[rank].nsend) {
      sc_req_tbl[rank].nsend = 1;
      sc_req_tbl[rank].send_reqs = (MPI_Request *)malloc(sizeof(MPI_Request));
      sc_req_tbl[rank].send_bufs = (void **)malloc(sizeof(void *));
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
  sc_debug("INFO", "%s exit", __FUNCTION__);
}


int send_int(int *vals, int count, role *r, int label)
{
  sc_debug("SEND", "%s { count:%d to:%s }", __FUNCTION__, count, r->type == SESSION_ROLE_P2P ? r->p2p->name: r->grp->name);

  int rc = 0;
  MPI_Request req = NULL;
  int *buf = (int *)malloc(sizeof(int) * count);
  memcpy(buf, vals, sizeof(int) * count);

  // Am I Parametrised or non-parametrised?
  // Inspect r->s->myindex, ie. self reference { >= 0 | < 0 }

  if (r->s->myindex >= 0) { // Parametrised

    // Is target Parametrised or non-parametrised?
    // Inspect r->type { S_R_P2P | S_R_GRP } + r->name contains []
    switch (r->type) {
      case SESSION_ROLE_P2P:
        if (0 == strcmp(r->p2p->basename, r->p2p->name)) { // Non-parametrised

          // Mixed gather
          sc_debug("SEND.DUMMY", "Mixed Gather: %s ==> %s(root)", r->s->name, r->p2p->name);
          // Note: we cannot use the P2P role directly
          //       because it is not part of our gather group
          //       However, we use this to extract the intention
          //       ie. Gather, so we can lookup the relevant group
          role *g = r->s->param(r->s, r->s->basename);
          rc = MPI_Gather(buf, count, MPI_INT, NULL, count, MPI_INT, g->grp->root, g->grp->comm);

        } else if (0 == strcmp(r->p2p->basename, r->s->basename)) { // Parametrised, same role

          // Parametrised send
          sc_debug("SEND.DUMMY", "Parametrised P2P Send: %s ==> %s", r->s->name, r->p2p->name);
          rc = MPI_Isend(buf, count, MPI_INT, r->p2p->rank, label, r->p2p->comm, &req);

        } else {
          sc_error("%s: Parametrised P2P role (%s/%s) cannot send to Parametrised P2P role with different basename (%s/%s)",
              __FUNCTION__, r->s->name, r->s->basename, r->p2p->name, r->p2p->basename);
          return SC_ERR_ROLEINVAL;
        }

        break;
      case SESSION_ROLE_GRP:
        sc_error("%s: Parametrised P2P role cannot send to a GRP role", __FUNCTION__);
        return SC_ERR_ROLEINVAL;
        break;
    }

  } else { // Non-parametrised

    // Is target Parametrised or non-parametrised?
    // Inspect r->type { S_R_P2P | S_R_GRP }
    switch (r->type) {
      case SESSION_ROLE_P2P:
        if (0 == strcmp(r->p2p->basename, r->s->basename)) { // Non-parametrised

          // Normal send
          sc_debug("SEND.DUMMY", "Normal Send: %s ==> %s", r->s->name, r->p2p->name);
          rc = MPI_Isend(buf, count, MPI_INT, r->p2p->rank, label, r->p2p->comm, &req);

        } else { // Parametrised

          sc_error("%s: Non-parametrised P2P role cannot send to Parametrised P2P role", __FUNCTION__);

        }

        break;

      case SESSION_ROLE_GRP: // Mixed Scatter
        sc_debug("SEND.DUMMY", "Mixed Scatter: %s ==> %s", r->s->name, r->grp->name);
        int nproc;
        MPI_Comm_size(r->grp->comm, &nproc);
        buf = (int *)realloc(buf, sizeof(int) * nproc);
        memcpy(buf, vals, sizeof(int) * nproc);
        rc = MPI_Scatter(buf, count, MPI_INT, NULL, count, MPI_INT, r->grp->root, r->grp->comm);
        break;
    }

  }

  // Error handling
  if (MPI_SUCCESS != rc) {
    char errstr[MPI_MAX_ERROR_STRING];
    int errstr_length;
    MPI_Error_string(rc, errstr, &errstr_length);
    sc_error("%s: %s", __FUNCTION__, errstr);
  }

  if (NULL == req) {
    free(buf);
  } else {
    sc_manage_request(r->p2p->rank, (void *)buf, req);
  }

  sc_debug("INFO", "%s: finished with rc=%d", __FUNCTION__, rc);
  return rc;
}


int recv_int(int *arr, int count, role *r, int match_label)
{
  sc_debug("RECV", "%s { count:%d from:%s }", __FUNCTION__, count, r->type == SESSION_ROLE_P2P ? r->p2p->name: r->grp->name);

  int rc = 0;
  MPI_Status stat;

  // Am I Parametrised or non-parametrised?
  // Inspect r->s->myindex, ie. self reference { >= 0 | < 0 }

  if (r->s->myindex >= 0) { // Parametrised

    // Is target Parametrised or non-parametrised?
    // Inspect r->type { S_R_P2P | S_R_GRP } + r->name contains []
    switch (r->type) {
      case SESSION_ROLE_P2P:
        if (0 == strcmp(r->p2p->basename, r->p2p->name)) { // Non-parametrised

          // Mixed Scatter (receiving side)
          sc_debug("RECV.DUMMY", "Mixed Scatter: %s(root) ==> %s", r->p2p->name, r->s->name);
          // Note: we cannot use the P2P role directly
          //       because it is not part of our scatter group
          //       However, we use this to extract the intension
          //       ie. Scatter, so we can lookup the relevant group
          role *g = r->s->param(r->s, r->s->basename);
          MPI_Scatter(NULL, count, MPI_INT, arr, count, MPI_INT, g->grp->root, g->grp->comm);

        } else if (0 == strcmp(r->p2p->basename, r->s->basename)) { // Parametrised, same role

          // Parametrised Recv
          sc_debug("RECV.DUMMY", "Parametrised P2P Recv: %s ==> %s", r->p2p->name, r->s->name);
          rc = MPI_Recv(arr, count, MPI_INT, r->p2p->rank, match_label, r->p2p->comm, &stat);

        } else {
          sc_error("%s: Parametrised P2P role (%s/%s) cannot receive from Parametrised P2P role with different basename (%s/%s)",
              __FUNCTION__, r->s->name, r->s->basename, r->p2p->name, r->p2p->basename);
          return SC_ERR_ROLEINVAL;
        }
        break;

      case SESSION_ROLE_GRP:
        sc_error("%s: Parametrised P2P role cannot receive from a GRP role", __FUNCTION__);
        return SC_ERR_ROLEINVAL;
        break;
    }

  } else { // Non-parametrised

    // Is target Parametrised or non-parametrised?
    // Inspect r->type { S_R_P2P | S_R_GRP }
    switch (r->type) {
      case SESSION_ROLE_P2P:
        if (0 == strcmp(r->p2p->basename, r->s->basename)) { // Non-parametrised

          // Normal recv
          sc_debug("RECV.DUMMY", "Normal Recv: %s ==> %s", r->p2p->name, r->s->name);
          rc = MPI_Recv(arr, count, MPI_INT, r->p2p->rank, match_label, r->p2p->comm, &stat);

        } else { // Parametrised

          sc_error("%s: Non-parametrised P2P role cannot receive from Parametrised P2P role", __FUNCTION__);

        }
        break;

      case SESSION_ROLE_GRP: // Mixed Gather
        sc_debug("RECV.DUMMY", "Mixed Gather: %s ==> %s(root) | Assumes nproc * count space allocated", r->grp->name, r->s->name);
        MPI_Gather(NULL, count, MPI_INT, arr, count, MPI_INT, r->grp->root, r->grp->comm);
        break;
    }

  }

  // Error handling
  if (MPI_SUCCESS != rc) {
    char errstr[MPI_MAX_ERROR_STRING];
    int errstr_length;
    MPI_Error_string(rc, errstr, &errstr_length);
    sc_error("%s: %s", __FUNCTION__, errstr);
  }
//  sc_manage_request(r->p2p->rank, NULL, NULL);

  sc_debug("INFO", "%s: finished with rc=%d", __FUNCTION__, rc);
  return rc;
}


int alltoall_int(int *vals, int count, role *g)
{
  sc_debug("ALL2ALL", "%s { count:%d }", __FUNCTION__, count);
  int rc = 0;
  if (SESSION_ROLE_GRP != g->type) {
    sc_error("%s: Cannot perform AllToAll on non-group role", __FUNCTION__);
    return SC_ERR_ROLEINVAL;
  }
  sc_debug("ALL2ALL.DUMMY", "All to all: %s parametrised role | Assumes g.size memory allocated", g->grp->name);
  int offset = sizeof(int) * count * (g->s->myindex-1);
  if (offset != 0) memcpy(&vals[offset], vals, count * sizeof(int));
  rc = MPI_Alltoall(vals, count, MPI_INT, vals, count, MPI_INT, g->grp->comm);

  // Error handling
  if (MPI_SUCCESS != rc) {
    char errstr[MPI_MAX_ERROR_STRING];
    int errstr_length;
    MPI_Error_string(rc, errstr, &errstr_length);
    sc_error("%s: %s", __FUNCTION__, errstr);
  }

  sc_debug("INFO", "%s: finished with rc=%d", __FUNCTION__, rc);
  return rc;
}


/* ----------------- Floats ----------------- */


int send_double(double *vals, int count, role *r, int label)
{
  sc_debug("SEND", "%s { count:%d to:%s }", __FUNCTION__, count, r->type == SESSION_ROLE_P2P ? r->p2p->name: r->grp->name);

  int rc = 0;
  MPI_Request req = NULL;
  int sz;
  double *buf = (double *)malloc(sizeof(double) * count);
  memcpy(buf, vals, sizeof(double) * count);

  // Am I Parametrised or non-parametrised?
  // Inspect r->s->myindex, ie. self reference { >= 0 | < 0 }

  if (r->s->myindex >= 0) { // Parametrised

    // Is target Parametrised or non-parametrised?
    // Inspect r->type { S_R_P2P | S_R_GRP } + r->name contains []
    switch (r->type) {
      case SESSION_ROLE_P2P:
        if (0 == strcmp(r->p2p->basename, r->p2p->name)) { // Non-parametrised

          // Mixed gather
          sc_debug("SEND.DUMMY", "Mixed Gather: %s ==> %s(root)", r->s->name, r->p2p->name);
          // Note: we cannot use the P2P role directly
          //       because it is not part of our gather group
          //       However, we use this to extract the intention
          //       ie. Gather, so we can lookup the relevant group
          role *g = r->s->param(r->s, r->s->basename);
          rc = MPI_Gather(buf, count, MPI_DOUBLE, NULL, count, MPI_DOUBLE, g->grp->root, g->grp->comm);

        } else if (0 == strcmp(r->p2p->basename, r->s->basename)) { // Parametrised, same role

          // Parametrised send
          sc_debug("SEND.DUMMY", "Parametrised P2P Send: %s ==> %s", r->s->name, r->p2p->name);
          rc = MPI_Isend(buf, count, MPI_DOUBLE, r->p2p->rank, label, r->p2p->comm, &req);

        } else {
          sc_error("%s: Parametrised P2P role (%s/%s) cannot send to Parametrised P2P role with different basename (%s/%s)",
              __FUNCTION__, r->s->name, r->s->basename, r->p2p->name, r->p2p->basename);
          return SC_ERR_ROLEINVAL;
        }

        break;
      case SESSION_ROLE_GRP:
        sc_debug("SEND.DUMMY", "Parametrised P2P role Broadcast %s ==> %s GRP role", r->s->name, r->grp->name);
        rc = MPI_Bcast(buf, count, MPI_DOUBLE, r->grp->root, r->grp->comm);
        break;
    }

  } else { // Non-parametrised

    // Is target Parametrised or non-parametrised?
    // Inspect r->type { S_R_P2P | S_R_GRP }
    switch (r->type) {
      case SESSION_ROLE_P2P:
        if (0 == strcmp(r->p2p->basename, r->s->basename)) { // Non-parametrised

          // Normal send
          sc_debug("SEND.DUMMY", "Normal Send: %s ==> %s", r->s->name, r->p2p->name);
          rc = MPI_Isend(buf, count, MPI_DOUBLE, r->p2p->rank, label, r->p2p->comm, &req);

        } else { // Parametrised

          sc_error("%s: Non-parametrised P2P role cannot send to Parametrised P2P role", __FUNCTION__);

        }

        break;

      case SESSION_ROLE_GRP: // Mixed Scatter
        sc_debug("SEND.DUMMY", "Mixed Scatter: %s ==> %s", r->s->name, r->grp->name);
        int nproc;
        MPI_Comm_size(r->grp->comm, &nproc);
        buf = (double *)realloc(buf, sizeof(double) * nproc);
        memcpy(buf, vals, sizeof(int) * nproc);
        rc = MPI_Scatter(buf, count, MPI_DOUBLE, NULL, count, MPI_DOUBLE, r->grp->root, r->grp->comm);
        break;
    }

  }

  // Error handling
  if (MPI_SUCCESS != rc) {
    char errstr[MPI_MAX_ERROR_STRING];
    int errstr_length;
    MPI_Error_string(rc, errstr, &errstr_length);
    sc_error("%s: %s", __FUNCTION__, errstr);
  }

  if (NULL == req) {
    free(buf);
  } else {
    if (r->type == SESSION_ROLE_P2P) sc_manage_request(r->p2p->rank, (void *)buf, req);
  }

  sc_debug("INFO", "%s: finished with rc=%d", __FUNCTION__, rc);
  return rc;
}


int recv_double(double *arr, int count, role *r, int match_label)
{
  sc_debug("RECV", "%s { count:%d from:%s }", __FUNCTION__, count, r->type == SESSION_ROLE_P2P ? r->p2p->name: r->grp->name);

  int rc = 0;
  MPI_Status stat;
  int sz; 

  // Am I Parametrised or non-parametrised?
  // Inspect r->s->myindex, ie. self reference { >= 0 | < 0 }

  if (r->s->myindex >= 0) { // Parametrised

    // Is target Parametrised or non-parametrised?
    // Inspect r->type { S_R_P2P | S_R_GRP } + r->name contains []
    switch (r->type) {
      case SESSION_ROLE_P2P:
        if (0 == strcmp(r->p2p->basename, r->p2p->name)) { // Non-parametrised

          // Mixed Scatter (receiving side)
          sc_debug("RECV.DUMMY", "Mixed Scatter: %s(root) ==> %s", r->p2p->name, r->s->name);
          // Note: we cannot use the P2P role directly
          //       because it is not part of our scatter group
          //       However, we use this to extract the intension
          //       ie. Scatter, so we can lookup the relevant group
          role *g = r->s->param(r->s, r->s->basename);
          MPI_Scatter(NULL, count, MPI_DOUBLE, arr, count, MPI_DOUBLE, g->grp->root, g->grp->comm);

        } else if (0 == strcmp(r->p2p->basename, r->s->basename)) { // Parametrised, same role

          // Parametrised Recv
          sc_debug("RECV.DUMMY", "Parametrised P2P Recv: %s ==> %s", r->p2p->name, r->s->name);
          rc = MPI_Recv(arr, count, MPI_DOUBLE, r->p2p->rank, match_label, r->p2p->comm, &stat);

        } else {
          sc_error("%s: Parametrised P2P role (%s/%s) cannot receive from Parametrised P2P role with different basename (%s/%s)",
              __FUNCTION__, r->s->name, r->s->basename, r->p2p->name, r->p2p->basename);
          return SC_ERR_ROLEINVAL;
        }
        break;

      case SESSION_ROLE_GRP:
        sc_debug("RECV.DUMMY", "Parametrised P2P role Broadcast GRP %s ==> P2P %s (Root: %d)", r->grp->name, r->s->name, r->grp->root, sz);
        rc = MPI_Bcast(arr, count, MPI_DOUBLE, r->grp->root, r->grp->comm);
        break;
    }

  } else { // Non-parametrised

    // Is target Parametrised or non-parametrised?
    // Inspect r->type { S_R_P2P | S_R_GRP }
    switch (r->type) {
      case SESSION_ROLE_P2P:
        if (0 == strcmp(r->p2p->basename, r->s->basename)) { // Non-parametrised

          // Normal recv
          sc_debug("RECV.DUMMY", "Normal Recv: %s ==> %s", r->p2p->name, r->s->name);
          rc = MPI_Recv(arr, count, MPI_DOUBLE, r->p2p->rank, match_label, r->p2p->comm, &stat);

        } else { // Parametrised

          sc_error("%s: Non-parametrised P2P role cannot receive from Parametrised P2P role", __FUNCTION__);

        }
        break;

      case SESSION_ROLE_GRP: // Mixed Gather
        sc_debug("RECV.DUMMY", "Mixed Gather: %s ==> %s(root) | Assumes nproc * count space allocated", r->grp->name, r->s->name);
        MPI_Gather(NULL, count, MPI_DOUBLE, arr, count, MPI_DOUBLE, r->grp->root, r->grp->comm);
        break;
    }

  }

  // Error handling
  if (MPI_SUCCESS != rc) {
    char errstr[MPI_MAX_ERROR_STRING];
    int errstr_length;
    MPI_Error_string(rc, errstr, &errstr_length);
    sc_error("%s: %s", __FUNCTION__, errstr);
  }
  if (r->type == SESSION_ROLE_P2P) sc_manage_request(r->p2p->rank, NULL, NULL);

  sc_debug("INFO", "%s: finished with rc=%d", __FUNCTION__, rc);
  return rc;
}


int alltoall_double(double *vals, int count, role *g)
{
  sc_debug("ALL2ALL", "%s { count:%d }", __FUNCTION__, count);
  int rc = 0;
  if (SESSION_ROLE_GRP != g->type) {
    sc_error("%s: Cannot perform AllToAll on non-group role", __FUNCTION__);
    return SC_ERR_ROLEINVAL;
  }
  sc_debug("ALL2ALL.DUMMY", "All to all: %s parametrised role | Assumes g.size memory allocated", g->grp->name);
  int offset = sizeof(int) * count * (g->s->myindex-1);
  if (offset != 0) memcpy(&vals[offset], vals, count * sizeof(int));
  rc = MPI_Alltoall(vals, count, MPI_DOUBLE, vals, count, MPI_DOUBLE, g->grp->comm);

  // Error handling
  if (MPI_SUCCESS != rc) {
    char errstr[MPI_MAX_ERROR_STRING];
    int errstr_length;
    MPI_Error_string(rc, errstr, &errstr_length);
    sc_error("%s: %s", __FUNCTION__, errstr);
  }

  sc_debug("INFO", "%s: finished with rc=%d", __FUNCTION__, rc);
  return rc;
}

int barrier(session *s, int mode)
{
  sc_debug("BARRIER", "Before buffersync");
  if (1 == mode || 2 == mode) {
    int role_idx = 0;
    for (role_idx=0; role_idx<s->nrole; ++role_idx) {
      if (s->roles[role_idx]->type == SESSION_ROLE_P2P && sc_req_tbl[role_idx].nsend > 0) {
        MPI_Waitall(sc_req_tbl[s->roles[role_idx]->p2p->rank].nsend, sc_req_tbl[s->roles[role_idx]->p2p->rank].send_reqs, MPI_STATUSES_IGNORE);
      }
    }
  }
  sc_debug("BARRIER", "Before MPI_Barrier");
  if (2 == mode) return 0;
  return MPI_Barrier(s->roles[0]->p2p->comm);
}
