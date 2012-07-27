/**
 * \file
 * Session C runtime library (libsc)
 * session handling module for MPI backend.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <mpi.h>

#include "st_node.h"

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


/**
 * Helper function to lookup a role in a session.
 *
 */
static role *find_role_in_session(session *s, char *role_name)
{
  int role_idx;
#ifdef __DEBUG__
  fprintf(stderr, "%s: { role: %s } in ", __FUNCTION__, role_name);
  session_dump();
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


void sessoin_init(int *argc, char ***argv, session **s, const char *scribble)
{
  int role_idx, rank_idx;
#ifdef __DEBUG__
  sc_print_version();
  DEBUG_prig_start_time = sc_time();
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

  *s = (session *)malloc(sizeof(session));
  session *sess = *s;

  MPI_Init(argc, argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &sess->rank);
  MPI_Comm_size(MPI_COMM_WORLD, &sess->nrole);

  int total_ranks = 0;
  // First run: calculate number of roles we have (incl. parametrised)
  for (role_idx=0; role_idx<tree->info->nrole; ++role_idx) {
    if (tree->info->roles[role_idx]->idxcount > 0) {
      total_ranks += tree->info->roles[role_idx]->idxcount;
    } else {
      total_ranks += 1;
    }
  }

  sess->roles = (role **)calloc(sizeof(role *), total_ranks);
  // Second run: assign ranks to roles
  for (role_idx=0, rank_idx=0; role_idx<tree->info->nrole; ++role_idx) {
    assert(rank_idx < total_ranks);
    sess->roles[rank_idx] = (role *)malloc(sizeof(role));
    sess->roles[rank_idx]->type = SESSION_ROLE_P2P;
    sess->roles[rank_idx]->s = sess;
    sess->roles[rank_idx]->p2p = (struct role_endpoint *)malloc(sizeof(struct role_endpoint));

    if (tree->info->roles[role_idx]->idxcount > 0) {
      int p_role_idx = 0;
      for (p_role_idx=0; p_role_idx < tree->info->roles[role_idx]->idxcount; ++p_role_idx) {
        sess->roles[rank_idx]->p2p->name = (char *)calloc(sizeof(char), strlen(tree->info->roles[role_idx]->name)+2+(int)ceil(log10(p_role_idx))+1);
        sprintf(sess->roles[rank_idx]->p2p->name, "%s[%d]", tree->info->roles[role_idx]->name, p_role_idx);
        rank_idx++;
      }
    } else {
      sess->roles[rank_idx]->p2p->name = (char *)calloc(sizeof(char), strlen(tree->info->roles[role_idx]->name)+1);
      strcpy(sess->roles[rank_idx]->p2p->name, tree->info->roles[role_idx]->name);
      rank_idx++;
    }
  }

  sess->r = &find_role_in_session;

  free(tree);
#ifdef __DEBUG__
  DEBUG_sess_start_time = sc_time();
#endif
}


void session_end(session *s)
{
  int role_idx;
  int role_count = s->nrole;

#ifdef __DEBUG__
  DEBUG_sess_end_time = sc_time();
#endif

  MPI_Finalize();
  for (role_idx=0; role_idx<role_count; role_idx++) {
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

  printf("\n------Session-------\n");
  printf("My role: %s\n", s->name);
  printf("Number of endpoint roles: %u\n", s->nrole);

  for (endpoint_idx=0; endpoint_idx<endpoint_count; endpoint_idx++) {
    switch (s->roles[endpoint_idx]->type) {
      case SESSION_ROLE_P2P:
        assert(s->roles[endpoint_idx]->p2p != NULL);
        printf("Endpoint#%u { type: p2p, name: %s, rank: %d }\n",
          endpoint_idx,
          s->roles[endpoint_idx]->p2p->name,
          s->roles[endpoint_idx]->p2p->rank);
        break;
      case SESSION_ROLE_GRP:
        assert(s->roles[endpoint_idx]->grp != NULL);
        printf("Endpoint#%u { type: group, name: %s }\n",
          endpoint_idx,
          s->roles[endpoint_idx]->grp->name);
        break;
      default:
        printf("Endpoint#%u { type: unknown }\n", endpoint_idx);
    }
  }
  printf("--------------------\n");
}
