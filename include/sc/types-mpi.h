#ifndef SC__TYPES_MPI_H__
#define SC__TYPES_MPI_H__
/**
 * \file
 * Session C runtime library MPI backend (libsc-mpi)
 * type definitions (MPI version).
 */

#include "mpi.h"

#define SESSION_ROLE_P2P 0
#define SESSION_ROLE_GRP 1


/**
 * For request tracking.
 *
 */
typedef struct {
  int nsend;
  MPI_Request *send_reqs;
  void **send_bufs;
} sc_req_tbl_t;


struct role_endpoint
{
  char *name;
  char *basename;
  int rank;
  MPI_Comm comm; // MPI Communicator
};


struct role_group
{
  char *name;
  int root; // The only non-parametrised role
  MPI_Comm comm; // MPI Communicator
  MPI_Group group;
};


/**
 * An endpoint.
 *
 * Endpoint is represented as a composite type:
 * (1) Basic point-to-point role.
 * (2) Group of roles.
 *
 * Internally, role is communicator + rank,
 * and group is communicator.
 */
struct role_t
{
  struct session_t *s;
  int type;

  union {
    struct role_endpoint *p2p;
    struct role_group *grp;
  };
};

typedef struct role_t role;


/**
 * An endpoint session.
 *
 * Holds all information on an
 * instantiated endpoint protocol.
 */
struct session_t
{
  int nrole; // # of roles (expanded) in session (size)
  role **roles; // Pointers to roles in session
  char *name;
  char *basename;
  int myindex;
  int *coords;
  int _myrank; // My rank, don't use this directly

  // Lookup functions.
  role *(*role)(struct session_t *s, char *name); // P2P and builtin groups
  role *(*rolei)(struct session_t *s, char *name, int index); // Index addressing
  role *(*idx)(struct session_t *s, int offset); // Index offset
  role *(*coord)(struct session_t *s, int *offset_vec); // Vector offset
  role *(*param)(struct session_t *s, char *param_name); // Parametrised groups
  role *(*parami)(struct session_t *s, char *param_name, int index); // Parametrised groups index addressing
};

typedef struct session_t session;

#endif // SC__TYPES_MPI_H__
