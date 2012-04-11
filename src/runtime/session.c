#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <zmq.h>

#include "connmgr.h"
#include "st_node.h"

#include "sc.h"
#include "sc/session.h"

extern FILE *yyin;
extern int yyparse();

/**
 * Helper function to lookup a role in a session.
 */
role *find_role_in_session(session *s, char *role_name)
{
  int role_idx;
  for (role_idx=0; role_idx<s->nrole; ++role_idx) {
    switch (s->roles[role_idx]->type) {
      case SESSION_ROLE_P2P: 
        if (strcmp(s->roles[role_idx]->p2p->name, role_name) == 0) {
          return s->roles[role_idx]->p2p->ptr;
        }
        break;
      case SESSION_ROLE_NAMED:
        assert(0); // TODO handle named endpoint
        break;
      case SESSION_ROLE_INDEXED:
        assert(0); // TODO handle indexed endpoint
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


void session_init(int *argc, char ***argv, session **s, const char *scribble)
{
  unsigned int role_idx;
#ifdef __DEBUG__
  fprintf(stdout, "Session C version %d.%d.%d\n", SESSIONC_MAJOR, SESSIONC_MINOR, SESSIONC_PATCH);
#endif

  st_tree *tree = st_tree_init((st_tree *)malloc(sizeof(st_tree)));

  // Get meta information from Scribble protocol.
  if ((yyin = fopen(scribble, "r")) == NULL) {
    fprintf(stderr, "Warning: Cannot open %s, reading from stdin\n", scribble);
  }
  yyparse(tree);

#ifdef __DEBUG__
  st_tree_print(tree);
#endif


  // Sanity check.
  if (tree->info->global) {
    fprintf(stderr, "Error: %s is a Global protocol\n", scribble);
    return;
  }

  // Parse arguments.
  int option;
  char *config_file = NULL;
  char *hosts_file = NULL;
  char *protocol_file = NULL;

  // Invoke getopt to extract arguments we need
  while (1) {
    static struct option long_options[] = {
      {"conf",     required_argument, 0, 'c'},
      {"hosts",    required_argument, 0, 's'},
      {"protocol", required_argument, 0, 'p'},
      {0, 0, 0, 0}
    };

    int option_idx = 0;
    option = getopt_long(*argc, *argv, "c:s:p:", long_options, &option_idx);

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

    nconns = connmgr_init(&conns, &hosts_roles, roles, nroles, hosts, nhosts, 2048);

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

  // Direct connections (p2p).
  sess->nrole = tree->info->nrole;
  sess->roles = (role **)malloc(sizeof(role *) * sess->nrole);
  sess->ctx = zmq_init(1);

  for (role_idx=0; role_idx<sess->nrole; role_idx++) {
    sess->roles[role_idx] = (role *)malloc(sizeof(role));
    sess->roles[role_idx]->type = SESSION_ROLE_P2P;
    sess->roles[role_idx]->s = sess;
    sess->roles[role_idx]->p2p = (struct role_endpoint *)malloc(sizeof(struct role_endpoint));

    sess->roles[role_idx]->p2p->name = (char *)calloc(sizeof(char), strlen(tree->info->roles[role_idx])+1);
    strcpy(sess->roles[role_idx]->p2p->name, tree->info->roles[role_idx]);

    for (conn_idx=0; conn_idx<nconns; conn_idx++) { // Look for matching connection parameter

      if (strcmp(conns[conn_idx].to, sess->roles[role_idx]->p2p->name) == 0) { // As a client.
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

      if (strcmp(conns[conn_idx].from, sess->roles[role_idx]->p2p->name) == 0) { // As a server.
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

  sess->r = &find_role_in_session;

  free(tree);
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

  sleep(1);

  for (role_idx=0; role_idx<role_count; role_idx++) {
    switch (s->roles[role_idx]->type) {
      case SESSION_ROLE_P2P:
        assert(s->roles[role_idx]->p2p != NULL);
        if (zmq_close(s->roles[role_idx]->p2p->ptr)) {
          perror("zmq_close");
        }
        break;
      case SESSION_ROLE_NAMED:
        assert(0); // TODO handle named endpoint
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
        printf("Endpoint#%u { type: p2p, name: %s, uri: %p %s }\n",
          endpoint_idx,
          s->roles[endpoint_idx]->p2p->name,
          &s->roles[endpoint_idx]->p2p->uri,
          s->roles[endpoint_idx]->p2p->uri);
        break;
      case SESSION_ROLE_NAMED:
        assert(s->roles[endpoint_idx]->group != NULL);
        printf("Endpoint#%u { type: group, name: %s, endpoints: [\n",
          endpoint_idx,
          s->roles[endpoint_idx]->group->name);
        unsigned int grp_endpoint_idx;
        unsigned int grp_endpoint_count = s->roles[endpoint_idx]->group->nendpoint;
        for (grp_endpoint_idx=0; grp_endpoint_idx<grp_endpoint_count; grp_endpoint_idx++) {
          printf("  { name: %s, uri: %s }\n",
            s->roles[endpoint_idx]->group->endpoints[grp_endpoint_idx]->name,
            s->roles[endpoint_idx]->group->endpoints[grp_endpoint_idx]->uri);
        }
        printf("]}\n");
        break;
      default:
        printf("Endpoint#%u { type: unknown }\n", endpoint_idx);
    }
  }
  printf("--------------------\n");
}
