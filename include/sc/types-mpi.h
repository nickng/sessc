#ifndef SC__TYPES_MPI_H__
#define SC__TYPES_MPI_H__
/**
 * \file
 * Session C runtime library MPI backend (libsc-mpi)
 * type definitions (MPI version).
 */

#include <mpi.h>

#define SESSION_ROLE_P2P     0
#define SESSION_ROLE_GRP     1


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
  int rank;
  MPI_Comm comm; // MPI Communicator
};


struct role_group
{
  char *name;
  MPI_Comm comm; // MPI Communicator
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
    struct role_endpoint *grp;
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
  int is_parametrised;
  int myrole; // My rank

  // Lookup functions.
  role *(*r)(struct session_t *, char *); // P2P and Group role
};

typedef struct session_t session;

#endif // SC__TYPES_MPI_H__
