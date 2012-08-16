#ifndef SC__TYPES_H__
#define SC__TYPES_H__
/**
 * \file
 * Session C runtime library (libsc)
 * type definitions.
 */

#define SESSION_ROLE_P2P     0
#define SESSION_ROLE_GRP     1
#define SESSION_ROLE_INDEXED 2


struct role_endpoint
{
  char *name;
  void *ptr;
  char uri[6+255+7]; // tcp:// + FQDN + :port + \0
};

struct role_group
{
  char *name;
  unsigned int nendpoint;
  struct role_endpoint **endpoints;

  struct role_endpoint *in;  // SUB
  struct role_endpoint *out; // PUB
};

struct role_indexed
{
  char *name; // Role name of indexed role
  unsigned int nendpoint;
  int *index_map; // Maps array index (eg. 0..9) to role index (eg. 1..10)
  struct role_t **endpoints; // Pointers to role_endpoint roles
};

/**
 * An endpoint.
 *
 * Endpoint is represented as a composite type:
 * (1) Basic point-to-point role.
 * (2) Group of named p2p roles.
 * (3) Group of unnamed indexed roles.
 */
struct role_t
{
  struct session_t *s;
  int type;

  union {
    struct role_endpoint *p2p;
    struct role_group    *grp;
    struct role_indexed  *idx;
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
  unsigned int nrole; // # of roles in session
  role **roles; // Pointers to roles in session
  char *name;
  int is_parametrised;
  int index; // -1 if not parametrised

  // Lookup functions.
  role *(*r)(struct session_t *, char *); // P2P and Group role
  role *(*i)(struct session_t *, int); // Indexed role

  // Extra data.
  void *ctx;
};

typedef struct session_t session;


#endif // SC__TYPES_H__
