#ifndef SESSION_H__
#define SESSION_H__

#define SESSION_ROLE_P2P     0
#define SESSION_ROLE_NAMED   1
#define SESSION_ROLE_INDEXED 2


struct role_endpoint {
  char *name;
  void *ptr;
  char uri[6+255+7]; // tcp:// + FQDN + :port + \0
};

struct role_group {
  char *name;
  unsigned int nendpoint;
  struct role_endpoint **endpoints;

  struct role_endpoint *inptr;  // SUB
  struct role_endpoint *outptr; // PUB
};


/**
 * An endpoint.
 *
 * Endpoint is represented as a composite type:
 * (1) Basic point-to-point role.
 * (2) Group of named p2p roles.
 * (3) Group of unnamed indexed roles.
 */
struct role_t {
  struct session_t *s;
  int type;

  union {
    struct role_endpoint *p2p;
    struct role_group    *group;
  };
};

typedef struct role_t role;

/**
 * An endpoint session.
 *
 * Holds all information on an
 * instantiated endpoint protocol.
 */
struct session_t {
  unsigned int nrole;
  role **roles;
  char *name;

  // Lookup function.
  role *(*r)(struct session_t *, char *);

  // Extra data.
  void *ctx;
};

typedef struct session_t session;

/**
 * \brief Initialise a sesssion.
 *
 * @param[in,out] argc     Command line argument count
 * @param[in,out] argv     Command line argument list
 * @param[out]    s        Pointer to session varible to create
 * @param[in]     scribble Endpoint Scribble file path for this session
 *                         (Must be constant string)
 */
void session_init(int *argc, char ***argv, session **s, const char *scribble);

/**
 * \brief Create a role group.
 *
 * @param[in,out] s     Session the new role group to create in.
 * @param[in]     name  Name of the new role group.
 * @param[in]     nrole Number of roles to group together.
 * @param[in]     ...   The handles to existing roles to group together. 
 *
 * \returns Handle to the newly created role group.
 */
role *session_group(session *s, const char *name, int nrole, ...);


/**
 * \brief Terminate a session.
 *
 * @param[in] s Session to terminate
 */
void session_end(session *s);


/**
 * \brief Dump content of an established session.
 *  
 * @param[in] s Session to dump
 */
void session_dump(const session *s);


#endif // SESSION_H__
