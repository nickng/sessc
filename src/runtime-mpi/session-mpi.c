/**
 * \file
 * Session C runtime library MPI backend (libsc-mpi)
 * session handling module for.
 */

#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <mpi.h>

#include "st_node.h"

#include "sc/errors.h"
#include "sc/session-mpi.h"
#include "sc/types-mpi.h"
#include "sc/utils.h"


extern FILE *yyin;
extern int yyparse(st_tree *tree);

#ifdef __DEBUG__
long long DEBUG_prog_start_time;
long long DEBUG_sess_start_time;
long long DEBUG_sess_end_time;
long long DEBUG_prog_end_time;
#endif

sc_req_tbl_t* sc_req_tbl;


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


int session_init(int *argc, char ***argv, session **s, const char *scribble)
{
#ifdef __DEBUG__
  sc_print_version();
  DEBUG_prog_start_time = sc_time();
#endif

  st_tree *tree = st_tree_init((st_tree *)malloc(sizeof(st_tree)));
  st_tree *local_tree = st_tree_init((st_tree *)malloc(sizeof(st_tree)));

  //
  // Phase 1: Load roles information from global protocol.
  //

  // Use getopt to retrieve argument (-p GlobalProtocol.spr)
  int option;
  char *protocol_file = NULL;

  while (1) {
    static struct option long_options[] = {
      {"protocol", required_argument, 0, 'p'},
      {0, 0, 0, 0}
    };

    int option_idx = 0;
    option = getopt_long(*argc, *argv, "p:", long_options, &option_idx);

    if (option == -1) break;

    switch (option) {
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

  if (protocol_file == NULL) {
    protocol_file = "Protocol.spr";
    fprintf(stderr, "Warning: protocol file not specified (-p), reading from `%s'\n", protocol_file);
  }

  // Load global protocol.
  if ((yyin = fopen(protocol_file, "r")) == NULL) {
    fprintf(stderr, "Warning: Cannot open %s, reading from stdin\n", protocol_file);
  }
  yyparse(tree);

  assert(tree != NULL);
  assert(tree->info != NULL);

  if (ST_TYPE_GLOBAL != tree->info->type) {
    fprintf(stderr, "Error: %s is NOT a Global protocol\n", protocol_file);
    return SC_ERR_PROTNVAL;
  }


  //
  // Phase 2: Load 'my role' from endpoint protocol
  //          1) Check if protocol matches global protocol
  //          2) Check whether my role is parametrised
  //

  if ((yyin = fopen(scribble, "r")) == NULL) {
    fprintf(stderr, "Warning: Cannot open %s, reading from stdin\n", scribble);
  }
  yyparse(local_tree);

  if (ST_TYPE_GLOBAL == local_tree->info->type) { // ! NORMAL and ! PARAMETRISED
    fprintf(stderr, "Error: %s is NOT an Endpoint protocol\n", scribble);
    return SC_ERR_PROTNVAL;
  }


  //
  // Phase 3: Set up session using global and endpoint protocol
  //          1) Identify own rank
  //          2) Match roles -> ranks (symmetric in all endpoints)
  //          3) Verify own rank is expected rank (otherwise is launch error)
  //

  *s = (session *)malloc(sizeof(session));
  session *sess = *s;

  MPI_Init(argc, argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &sess->myrole); // My rank
  MPI_Comm_size(MPI_COMM_WORLD, &sess->nrole);  // Total number of available processes
  sess->is_parametrised = 0;
  sess->r = &find_role_in_session;

  assert(tree->info->nrole <= sess->nrole); // # roles <= available processes

  sess->roles = (role **)calloc(sizeof(role *), sess->nrole);

  // Note: Rank assignments for point-to-point roles is the `roles' array index for quick access.
  //       Group roles are given array index > nprocess

  int role_idx; // For iterating roles (from global protocol)
  int process_idx; // For iterating expanded roles (for session, = MPI rank)
  for (role_idx=0, process_idx=0; role_idx<tree->info->nrole; ++role_idx) {
    assert(process_idx < sess->nrole);
    assert(tree->info->roles[role_idx]->idxcount >= 0);
    printf("Role_idx: %d\n", role_idx);
    if (tree->info->roles[role_idx]->idxcount > 0) { // Is parametrised
      sess->is_parametrised |= 1;
      int param_role_idx;
      for (param_role_idx=0; param_role_idx < tree->info->roles[role_idx]->idxcount; ++param_role_idx) {
        sess->roles[process_idx] = (role *)malloc(sizeof(role));
        sess->roles[process_idx]->type = SESSION_ROLE_P2P;
        sess->roles[process_idx]->s = sess; // Self reference.

        sess->roles[process_idx]->p2p = (struct role_endpoint *)malloc(sizeof(struct role_endpoint));
        sess->roles[process_idx]->p2p->comm = MPI_COMM_WORLD;
        sess->roles[process_idx]->p2p->rank = process_idx;

        sess->roles[process_idx]->p2p->name
            = (char *)calloc(sizeof(char), strlen(tree->info->roles[role_idx]->name)+2+(int)ceil(log10(param_role_idx))+1);
        sprintf(sess->roles[process_idx]->p2p->name, "%s[%d]", tree->info->roles[role_idx]->name, param_role_idx);

        ++process_idx;
      }
    } else { // Is not parametrised
      sess->roles[process_idx] = (role *)malloc(sizeof(role));
      sess->roles[process_idx]->type = SESSION_ROLE_P2P;
      sess->roles[process_idx]->s = sess; // Self reference.

      sess->roles[process_idx]->p2p = (struct role_endpoint *)malloc(sizeof(struct role_endpoint));
      sess->roles[process_idx]->p2p->comm = MPI_COMM_WORLD;
      sess->roles[process_idx]->p2p->rank = process_idx;
      sess->roles[process_idx]->p2p->name = strdup(tree->info->roles[role_idx]->name);

      ++process_idx;
    }

#ifdef __DEBUG__
    fprintf(stderr, "Assigned: Rank %d = %s\n", sess->roles[process_idx-1]->p2p->rank, sess->roles[process_idx-1]->p2p->name);
#endif
  }

  // roles[] are now ready to access
  sess->name = sess->roles[sess->myrole]->p2p->name;


  //
  // Phase 4: Set up request table to track asynchronous requests/buffer management
  //

  sc_req_tbl = (sc_req_tbl_t *)calloc(sizeof(sc_req_tbl_t), sess->nrole);

  int rank_idx;
  for (rank_idx=0; rank_idx<sess->nrole; ++rank_idx) {
    sc_req_tbl[rank_idx].nsend = 0;
  }

#ifdef __DEBUG__
  fprintf(stderr, "----------\n");
  fprintf(stderr, "Summary of %s\n", __FUNCTION__);
  fprintf(stderr, "----------\n");
  fprintf(stderr, "Total number of roles: %d\n", sess->nrole);
  fprintf(stderr, "Is session parametrised?: %s\n", sess->is_parametrised ? "Yes" : "No");
  fprintf(stderr, "My rank is %d (role %s)\n", sess->myrole, sess->roles[sess->myrole]->p2p->name);
  fprintf(stderr, "Global protocol:");
  st_tree_print(tree);
  fprintf(stderr, "Local protocol:");
  st_tree_print(local_tree);
  session_dump(sess);
#endif

  free(tree);
  free(local_tree);
#ifdef __DEBUG__
  DEBUG_sess_start_time = sc_time();
#endif
  return SC_SUCCESS;
}


void session_end(session *s)
{
  int role_idx;
  for (role_idx=0; role_idx<s->nrole; ++role_idx) {
    MPI_Status *stats = (MPI_Status *)calloc(sizeof(MPI_Status), sc_req_tbl[role_idx].nsend);
    MPI_Waitall(sc_req_tbl[role_idx].nsend, sc_req_tbl[role_idx].send_reqs, stats);
    int i;
    for (i=0; i<sc_req_tbl[role_idx].nsend; ++i) {
      free(sc_req_tbl[role_idx].send_bufs[i]);
    }
    free(sc_req_tbl[role_idx].send_bufs);
    free(sc_req_tbl[role_idx].send_reqs);
    free(stats);
  }
  free(sc_req_tbl);

#ifdef __DEBUG__
  DEBUG_sess_end_time = sc_time();
#endif

  MPI_Finalize();
  for (role_idx=0; role_idx<s->nrole; role_idx++) {
    free(s->roles[role_idx]);
  }
  free(s->roles);

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

  fprintf(stderr, "\n------Session-------\n");
  fprintf(stderr, "My role: %s (rank %d)\n", s->name, s->myrole);
  fprintf(stderr, "Number of endpoint roles: %u\n", s->nrole);

  for (endpoint_idx=0; endpoint_idx<endpoint_count; endpoint_idx++) {
    switch (s->roles[endpoint_idx]->type) {
      case SESSION_ROLE_P2P:
        assert(s->roles[endpoint_idx]->p2p != NULL);
        fprintf(stderr, "Endpoint#%u { type: p2p, name: %s, rank: %d }\n",
          endpoint_idx,
          s->roles[endpoint_idx]->p2p->name,
          s->roles[endpoint_idx]->p2p->rank);
        break;
      case SESSION_ROLE_GRP:
        assert(s->roles[endpoint_idx]->grp != NULL);
        fprintf(stderr, "Endpoint#%u { type: group, name: %s }\n",
          endpoint_idx,
          s->roles[endpoint_idx]->grp->name);
        break;
      default:
        fprintf(stderr, "Endpoint#%u { type: unknown }\n", endpoint_idx);
    }
  }
  printf("--------------------\n");
}
