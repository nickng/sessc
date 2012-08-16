/**
 * \file
 * Session C runtime library (libsc)
 * session handling module.
 */

#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <zmq.h>

#include "connmgr.h"
#include "st_node.h"

#include "sc/session.h"
#include "sc/types.h"
#include "sc/utils.h"


extern FILE *yyin;
extern int yyparse(st_tree *tree);
#ifdef __DEBUG__
long long DEBUG_prog_start_time;
long long DEBUG_sess_start_time;
long long DEBUG_sess_end_time;
long long DEBUG_prog_end_time;
#endif


/**
 * Helper function to lookup a role in a session.
 *
 */
static role *find_role_in_session(session *s, char *role_name)
{
  int role_idx;
#ifdef __DEBUG__
  fprintf(stderr, "%s: { role: %s } in ", __FUNCTION__, role_name);
  session_dump(s);
#endif
  for (role_idx=0; role_idx<s->nrole; ++role_idx) {
    switch (s->roles[role_idx]->type) {
      case SESSION_ROLE_P2P: 
        if (strcmp(s->roles[role_idx]->p2p->name, role_name) == 0) {
          return s->roles[role_idx];
        }
        break;
      case SESSION_ROLE_GRP:
        if (strcmp(s->roles[role_idx]->grp->name, role_name) == 0) {
          return s->roles[role_idx];
        }
        break;
      case SESSION_ROLE_INDEXED:
        fprintf(stderr, "Error: %s not defined for indexed role, use r_idx() instead.\n", __FUNCTION__);
        return NULL;
        break;
      default:
        fprintf(stderr, "Unknown endpoint type: %d\n", s->roles[role_idx]->type);
    } 
  }

  fprintf(stderr, "%s: Role %s not found in session.\n",
      __FUNCTION__, role_name);
#ifdef __DEBUG__
  session_dump(s);
#endif
  return NULL;
}


/**
 * Helper function to lookup an indexed role in a session.
 * This is used for index calculations, for direct reference of indexed roles,
 * use s->r(s, "IndexRole[42]") instead
 *
 */
static role *find_indexed_role_in_session(session *s, char *role_name, int index)
{
  int role_idx, endpoint_idx;
#ifdef __DEBUG__
  fprintf(stderr, "%s: { role: %s, index: %d } in ", __FUNCTION__, role_name, index);
  session_dump(s);
#endif
  for (role_idx=0; role_idx<s->nrole; ++role_idx) {
    switch (s->roles[role_idx]->type) {
      case SESSION_ROLE_P2P:
      case SESSION_ROLE_GRP:
        break;
      case SESSION_ROLE_INDEXED:
        if (0 == strcmp(s->roles[role_idx]->idx->name, role_name)) {
          for (endpoint_idx=0; endpoint_idx<s->roles[role_idx]->idx->nendpoint; ++endpoint_idx) {
            if (index == s->roles[role_idx]->idx->index_map[endpoint_idx]) {
              return s->roles[role_idx]->idx->endpoints[endpoint_idx];
            }
          }
          // Indexed role found but without matching index
          fprintf(stderr, "Error: Index not defined!\n");
        }
        break;
      default:
        fprintf(stderr, "Unknown endpoint type: %d\n", s->roles[role_idx]->type);
    }
  }

  fprintf(stderr, "%s: Role %s[%d] not found in session.\n",
      __FUNCTION__, role_name, index);
#ifdef __DEBUG__
  session_dump(s);
#endif
  return NULL;
}


void session_init(int *argc, char ***argv, session **s, const char *scribble)
{
  unsigned int role_idx;
#ifdef __DEBUG__
  sc_print_version();
  DEBUG_prog_start_time = sc_time();
#endif

  st_tree *tree = st_tree_init((st_tree *)malloc(sizeof(st_tree)));

  // Get meta information from Scribble protocol.
  if ((yyin = fopen(scribble, "r")) == NULL) {
    fprintf(stderr, "Warning: Cannot open %s, reading from stdin\n", scribble);
  }
  yyparse(tree);

#ifdef __DEBUG__
  printf("The local protocol:");
  st_tree_print(tree);
#endif


  // Sanity check.
  if (ST_TYPE_GLOBAL == tree->info->type) {
    fprintf(stderr, "Error: %s is a Global protocol\n", scribble);
    return;
  }

  // Parse arguments.
  int option;
  char *config_file = NULL;
  char *hosts_file = NULL;
  char *protocol_file = NULL;
  int my_role_index = -1;

  // Invoke getopt to extract arguments we need
  while (1) {
    static struct option long_options[] = {
      {"conf",     required_argument, 0, 'c'},
      {"hosts",    required_argument, 0, 's'},
      {"protocol", required_argument, 0, 'p'},
      {"index",    required_argument, 0, 'i'},
      {0, 0, 0, 0}
    };

    int option_idx = 0;
    option = getopt_long(*argc, *argv, "c:s:p:i:", long_options, &option_idx);

    if (option == -1) break;

    switch (option) {
      case 'c':
        config_file = (char *)calloc(sizeof(char), strlen(optarg)+1);
        strcpy(config_file, optarg);
        fprintf(stderr, "Using configuration file %s\n", config_file);
        break;
      case 's':
        hosts_file = (char *)calloc(sizeof(char), strlen(optarg)+1);
        strcpy(hosts_file, optarg);
        fprintf(stderr, "Using hosts file %s\n", hosts_file);
        break;
      case 'p':
        protocol_file = (char *)calloc(sizeof(char), strlen(optarg)+1);
        strcpy(protocol_file, optarg);
        fprintf(stderr, "Using protocol file %s\n", protocol_file);
        break;
      case 'i':
        my_role_index = atoi(optarg);
        fprintf(stderr, "Role index is %d\n", my_role_index);
        break;
    }
  }

  *argc -= optind-1;
  (*argv)[optind-1] = (*argv)[0];
  *argv += optind-1;

  conn_rec *conns;
  int nconns;
  char **hosts;
  int nhosts;
  char **roles;
  int nroles;
  host_map *hosts_roles;
  int conn_idx;

  if (config_file == NULL) { // Generate dynamic connection parameters (config file absent).

    if (hosts_file == NULL) {
      hosts_file = "hosts";
      fprintf(stderr, "Warning: host file not specified (-s), reading from `%s'\n", hosts_file);
    }
    nhosts = connmgr_load_hosts(hosts_file, &hosts);
    if (protocol_file == NULL) {
      protocol_file = "Protocol.spr";
      fprintf(stderr, "Warning: protocol file not specified (-p), reading from `%s'\n", protocol_file);
    }

    nroles = connmgr_load_roles(protocol_file, &roles);
    nconns = connmgr_init(&conns, &hosts_roles, roles, nroles, hosts, nhosts, 7777);

  } else { // Use config file.

    nconns = connmgr_read(config_file, &conns, &hosts_roles, &nroles);

  }

#ifdef __DEBUG__
  printf("\n------Conn Mgr------\n");
  connmgr_write("-", conns, nconns, hosts_roles, nroles);
  printf("--------------------\n");
#endif

  *s = (session *)malloc(sizeof(session));
  session *sess = *s;

  sess->name = (char *)calloc(sizeof(char), strlen(tree->info->myrole)+1);
  strcpy(sess->name, tree->info->myrole);

  sess->is_parametrised = 0; // Defaults to normal role
  if (tree->info->type == ST_TYPE_PARAMETRISED) {
    sess->is_parametrised = 1;
  }
  sess->index = my_role_index;

  if (sess->is_parametrised && sess->index == -1) {
    fprintf(stderr, "Warning: role index not specified, defaulting to 0\n");
    sess->index = 0;
  }

  // Direct connections (p2p).
  sess->nrole = tree->info->nrole;
  sess->roles = (role **)malloc(sizeof(role *) * sess->nrole);
  sess->ctx = zmq_init(1);

  for (role_idx=0; role_idx<sess->nrole; role_idx++) {
    sess->roles[role_idx] = (role *)malloc(sizeof(role));
    sess->roles[role_idx]->type = SESSION_ROLE_P2P;
    sess->roles[role_idx]->s = sess;
    sess->roles[role_idx]->p2p = (struct role_endpoint *)malloc(sizeof(struct role_endpoint));

    sess->roles[role_idx]->p2p->name = (char *)calloc(sizeof(char), strlen(tree->info->roles[role_idx]->name)+1);
    strcpy(sess->roles[role_idx]->p2p->name, tree->info->roles[role_idx]->name);

    for (conn_idx=0; conn_idx<nconns; conn_idx++) { // Look for matching connection parameter

      if ((CONNMGR_TYPE_P2P == conns[conn_idx].type) && strcmp(conns[conn_idx].to, sess->roles[role_idx]->p2p->name) == 0 && strcmp(conns[conn_idx].from, sess->name) == 0) { // As a client.
        assert(strlen(conns[conn_idx].host) < 255 && conns[conn_idx].port < 65536);
        if (strstr(conns[conn_idx].host, "ipc:") != NULL) {
          sprintf(sess->roles[role_idx]->p2p->uri, "ipc:///tmp/sessionc-%u", conns[conn_idx].port);
        } else {
          sprintf(sess->roles[role_idx]->p2p->uri, "tcp://%s:%u", conns[conn_idx].host, conns[conn_idx].port);
        }
#ifdef __DEBUG__
        fprintf(stderr, "Connection (as client) %s -> %s is %s\n",
            conns[conn_idx].from,
            conns[conn_idx].to,
            sess->roles[role_idx]->p2p->uri);
#endif
        if ((sess->roles[role_idx]->p2p->ptr = zmq_socket(sess->ctx, ZMQ_PAIR)) == NULL) perror("zmq_socket");
        if (zmq_connect(sess->roles[role_idx]->p2p->ptr, sess->roles[role_idx]->p2p->uri) != 0) perror("zmq_connect");

        break;
      }

      if ((CONNMGR_TYPE_P2P == conns[conn_idx].type) && strcmp(conns[conn_idx].from, sess->roles[role_idx]->p2p->name) == 0 && strcmp(conns[conn_idx].to, sess->name) == 0) { // As a server.
        assert(conns[conn_idx].port < 65536);
        if (strstr(conns[conn_idx].host, "ipc:") != NULL) {
          sprintf(sess->roles[role_idx]->p2p->uri, "ipc:///tmp/sessionc-%u", conns[conn_idx].port);
        } else {
          sprintf(sess->roles[role_idx]->p2p->uri, "tcp://*:%u",conns[conn_idx].port);
        }
  #ifdef __DEBUG__
        fprintf(stderr, "Connection (as server) %s -> %s is %s\n",
            conns[conn_idx].from,
            conns[conn_idx].to,
            sess->roles[role_idx]->p2p->uri);
  #endif
        if ((sess->roles[role_idx]->p2p->ptr = zmq_socket(sess->ctx, ZMQ_PAIR)) == NULL) perror("zmq_socket");
        if (zmq_bind(sess->roles[role_idx]->p2p->ptr, sess->roles[role_idx]->p2p->uri) != 0) perror("zmq_bind");

        break;
      }

    }

  }

  // Add a _Others group role.
  sess->nrole++;
  sess->roles = (role **)realloc(sess->roles, sizeof(role *) * sess->nrole);
  sess->roles[sess->nrole-1] = (role *)malloc(sizeof(role)); // A group role at roles[last_index]
  sess->roles[sess->nrole-1]->type = SESSION_ROLE_GRP;
  sess->roles[sess->nrole-1]->s = sess;
  sess->roles[sess->nrole-1]->grp = (struct role_group *)malloc(sizeof(struct role_group));

  sess->roles[sess->nrole-1]->grp->name = "_Others";
  sess->roles[sess->nrole-1]->grp->nendpoint = tree->info->nrole;
  sess->roles[sess->nrole-1]->grp->endpoints
      = (struct role_endpoint **)malloc(sizeof(struct role_endpoint *) * sess->roles[sess->nrole-1]->grp->nendpoint);
  // Copy endpoints to our global group role.
  unsigned int endpoint_idx;
  for (endpoint_idx=0; endpoint_idx<sess->roles[sess->nrole-1]->grp->nendpoint; endpoint_idx++) {
    if (sess->roles[endpoint_idx]->type == SESSION_ROLE_P2P) {
      sess->roles[sess->nrole-1]->grp->endpoints[endpoint_idx] = sess->roles[endpoint_idx]->p2p;
    }
  }

  sess->roles[sess->nrole-1]->grp->in  = (struct role_endpoint *)malloc(sizeof(struct role_endpoint));
  sess->roles[sess->nrole-1]->grp->out = (struct role_endpoint *)malloc(sizeof(struct role_endpoint));

  // Setup a SUB (broadcast-in) socket
  if ((sess->roles[sess->nrole-1]->grp->in->ptr = zmq_socket(sess->ctx, ZMQ_SUB)) == NULL) perror("zmq_socket");
  // Setup a series of PUB (broadcast-out) socket
  if ((sess->roles[sess->nrole-1]->grp->out->ptr = zmq_socket(sess->ctx, ZMQ_PUB)) == NULL) perror("zmq_socket");

  for (conn_idx=0; conn_idx<nconns; conn_idx++) { // Look for the broadcast socket
    if ((CONNMGR_TYPE_GRP == conns[conn_idx].type) && (strcmp(conns[conn_idx].to, sess->name) == 0)) {
      sprintf(sess->roles[sess->nrole-1]->grp->in->uri, "tcp://*:%u", conns[conn_idx].port);
#ifdef __DEBUG__
      fprintf(stderr, "Broadcast in-socket: %s\n",
        sess->roles[sess->nrole-1]->grp->in->uri);
#endif
      if (zmq_bind(sess->roles[sess->nrole-1]->grp->in->ptr, sess->roles[sess->nrole-1]->grp->in->uri) != 0) perror("zmq_bind");
      break;
    }
  }

  for (conn_idx=0; conn_idx<nconns; conn_idx++) { // Look for the broadcast socket
    if ((CONNMGR_TYPE_GRP == conns[conn_idx].type) && (strcmp(conns[conn_idx].to, sess->name) != 0)) {
      sprintf(sess->roles[sess->nrole-1]->grp->out->uri, "tcp://%s:%u", conns[conn_idx].host, conns[conn_idx].port);
#ifdef __DEBUG__
      fprintf(stderr, "Broadcast out-socket: %s\n",
        sess->roles[sess->nrole-1]->grp->out->uri);
#endif
      if (zmq_connect(sess->roles[sess->nrole-1]->grp->out->ptr, sess->roles[sess->nrole-1]->grp->out->uri) != 0) perror("zmq_connect");
    }
  }
  zmq_setsockopt(sess->roles[sess->nrole-1]->grp->in->ptr, ZMQ_SUBSCRIBE, "", 0);
#ifdef ZMQ_LINGER
  zmq_setsockopt(sess->roles[sess->nrole-1]->grp->out->ptr, ZMQ_LINGER, 0, 0);
#endif

  sess->r = &find_role_in_session;
  sess->r_idx = &find_indexed_role_in_session;

  free(tree);
#ifdef __DEBUG__
  DEBUG_sess_start_time = sc_time();
#endif
}


role *session_group(session *s, const char *name, int nrole, ...)
{
  assert(0);
  return NULL;
}


void session_end(session *s)
{
  unsigned int role_idx;
  unsigned int role_count = s->nrole;

#ifdef __DEBUG__
  DEBUG_sess_end_time = sc_time();
#endif

  sleep(1);

  for (role_idx=0; role_idx<role_count; role_idx++) {
    switch (s->roles[role_idx]->type) {
      case SESSION_ROLE_P2P:
        assert(s->roles[role_idx]->p2p != NULL);
        if (zmq_close(s->roles[role_idx]->p2p->ptr)) {
          perror("zmq_close");
        }
        break;
      case SESSION_ROLE_GRP:
        if (zmq_close(s->roles[role_idx]->grp->in->ptr) != 0) {
          perror("zmq_close");
        }
        if (zmq_close(s->roles[role_idx]->grp->out->ptr) != 0) {
          perror("zmq_close");
        }
        free(s->roles[role_idx]->grp->in);
        free(s->roles[role_idx]->grp->out);
        break;
      case SESSION_ROLE_INDEXED:
        assert(0); // TODO handle indexed endpoint
        break;
      default:
        fprintf(stderr, "Unable to terminate endpoint#%u: unknown type\n",
          role_idx);
    } // Switch
  }

  for (role_idx=0; role_idx<role_count; role_idx++) {
    free(s->roles[role_idx]);
  }
  free(s->roles);

  zmq_term(s->ctx);
  s->r = NULL;
  free(s);
#ifdef __DEBUG__
  DEBUG_prog_end_time = sc_time();
  printf("----- Statistics -----\n");
  printf("Total execution time (including session init and cleanup): %f sec\n", sc_time_diff(DEBUG_prog_start_time, DEBUG_prog_end_time));
  printf("Total time in session: %f sec\n", sc_time_diff(DEBUG_sess_start_time, DEBUG_sess_end_time));
  printf("----------------------\n");
#endif
}


void session_dump(const session *s)
{
  assert(s != NULL);
  unsigned int endpoint_idx;
  unsigned int endpoint_count = s->nrole;

  printf("\n------Session-------\n");
  printf("My role: %s\n", s->name);
  printf("Number of endpoint roles: %u\n", s->nrole);

  for (endpoint_idx=0; endpoint_idx<endpoint_count; endpoint_idx++) {
    switch (s->roles[endpoint_idx]->type) {
      case SESSION_ROLE_P2P:
        assert(s->roles[endpoint_idx]->p2p != NULL);
        printf("Endpoint#%u { type: p2p, name: %s, uri: %s }\n",
          endpoint_idx,
          s->roles[endpoint_idx]->p2p->name,
          s->roles[endpoint_idx]->p2p->uri);
        break;
      case SESSION_ROLE_GRP:
        assert(s->roles[endpoint_idx]->grp != NULL);
        printf("Endpoint#%u { type: group, name: %s, endpoints: [\n",
          endpoint_idx,
          s->roles[endpoint_idx]->grp->name);
        printf(" in { uri: %s }, out { uri: %s }\n",
          s->roles[endpoint_idx]->grp->in->uri,
          s->roles[endpoint_idx]->grp->out->uri);
        unsigned int grp_endpoint_idx;
        unsigned int grp_endpoint_count = s->roles[endpoint_idx]->grp->nendpoint;
        for (grp_endpoint_idx=0; grp_endpoint_idx<grp_endpoint_count; grp_endpoint_idx++) {
          printf("  { name: %s, uri: %s }\n",
            s->roles[endpoint_idx]->grp->endpoints[grp_endpoint_idx]->name,
            s->roles[endpoint_idx]->grp->endpoints[grp_endpoint_idx]->uri);
        }
        printf("]}\n");
        break;
      default:
        printf("Endpoint#%u { type: unknown }\n", endpoint_idx);
    }
  }
  printf("--------------------\n");
}
