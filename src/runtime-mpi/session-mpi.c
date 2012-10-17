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
#include <unistd.h>

#include <mpi.h>

#include "st_node.h"

#include "sc/debug.h"
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
static role *session_find_role(session *s, const char *name)
{
  int role_idx;
  role *r;
  sc_debug("INFO", "%s( role: %s ) in: ", __FUNCTION__, name);
  session_ddump(s);
  for (role_idx=0; role_idx<s->nrole; ++role_idx) {
    switch (s->roles[role_idx]->type) {
      case SESSION_ROLE_P2P:
        if (0 == strcmp(s->roles[role_idx]->p2p->name, name)) {
          sc_debug("INFO", "%s: Found %s { rank: %d }",
              __FUNCTION__, s->roles[role_idx]->p2p->name, s->roles[role_idx]->p2p->rank);
          r = s->roles[role_idx];
        }
        break;
      case SESSION_ROLE_GRP:
        if (0 == strcmp(s->roles[role_idx]->grp->name, name)) {
          sc_debug("INFO", "%s: Found %s", __FUNCTION__, s->roles[role_idx]->grp->name);
          r = s->roles[role_idx];
        }
        break;
      default:
        sc_error("Unknown endpoint type: %d", s->roles[role_idx]->type);
    }
  }
  if (NULL != r) return r;

  if (0 != strcmp(name, "__CACHEHACK__"))
    sc_error("%s: Role %s not found in session.", __FUNCTION__, name);
  session_ddump(s);
  return NULL;
}


static inline role *session__role(session *s, char *name)
{
  return session_find_role(s, name);
}


static inline role *session__rolei(session *s, char *name, int index)
{
  if (strlen(name) > 250) {
    sc_warn("%s: Supplied role name `%s' is too long!", __FUNCTION__, name);
  }
  char namei[255];
  sprintf(namei, "%s[%d]", name, index);
  return session_find_role(s, namei);
}


static role *session__idx(session *s, int offset)
{
  char *buf = strdup(s->name);
  char *oldname = strdup(strtok(buf, "["));
  int oldidx = atoi(strtok(NULL, "]"));
  free(buf);
  sc_debug("INFO", "%s: %s[%d] offset %d -> [%d]",
      __FUNCTION__, oldname, oldidx, offset, oldidx+offset);
  char name[256];
  memset(name, 0, 256);
  assert(strlen(s->name)+1<256/* Buffer too small */);
  sprintf(name, "%s[%d]", oldname, oldidx+offset);
  free(oldname);
  return session_find_role(s, name);
}


static inline role *session__coord(session *s, int *offset_vec)
{
  char *buf = strdup(s->name);
  char *oldname = strdup(strtok(buf, "["));
  free(buf);
  int coords[2];
  coords[0] = s->coords[0] + offset_vec[0];
  coords[1] = s->coords[1] + offset_vec[1];
  char name[255];
  sprintf(name, "%s[%d][%d]", oldname, coords[0], coords[1]);
  sc_debug("COORD", "(%d,%d) + (%d,%d) => %s[%d][%d]",
      s->coords[0], s->coords[1],
      offset_vec[0], offset_vec[1],
      oldname, coords[0], coords[1]);
  return session_find_role(s, name);
}


int sc_range(const session *s, int *from, int *to)
{
  int inrange = 1;
  int i;
  for (i=0; i<2; ++i) {
    inrange &= (from[i] <= s->coords[i]);
    inrange &= (s->coords[i] <= to[i]);
  }
  sc_debug("RNGCHK", "(%d,%d) <= (%d,%d) <= (%d,%d) -> %s",
      from[0], from[1],
      s->coords[0], s->coords[1],
      to[0], to[1],
      inrange?"Yes":"No");
  return inrange;
}


static role *session__param(session *s, char *param_name)
{
  int role_idx;
  role *r;
  sc_debug("INFO", "%s( grp: %s ) in: ", __FUNCTION__, param_name);
  session_ddump(s);
  for (role_idx=0; role_idx<s->nrole; ++role_idx) {
    switch (s->roles[role_idx]->type) {
      case SESSION_ROLE_GRP:
        if (0 == strcmp(s->roles[role_idx]->grp->name, param_name)) {
          sc_debug("INFO", "%s: Found %s", __FUNCTION__, s->roles[role_idx]->grp->name);
          r = s->roles[role_idx];
        }
        break;
      case SESSION_ROLE_P2P:
        break;
      default:
        sc_error("Unknown endpoint type: %d", s->roles[role_idx]->type);
    }
  }
  if (NULL != r) return r;
  sc_error("Group role %s not found", param_name);
  return NULL;
}


int session_init(int *argc, char ***argv, session **s, const char *endptprot)
{
  *s = (session *)malloc(sizeof(session));
  session *_sess = *s;

  st_tree *global_tree;
  st_tree *local_tree;
  int procid;
  int nproc;

  //
  // Phase 0: Standard MPI initialisation.
  //

  MPI_Init(argc, argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &procid);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);
#ifdef __DEBUG__
  sc_print_version();
  DEBUG_prog_start_time = sc_time();
#endif

  sc_debug("INIT.0", "MPI_Init OK, { rank = %d, size = %d }", procid, nproc);

  //
  // Phase 1: Local identification from endpoint protocol.
  //

  if ((yyin = fopen(endptprot, "r")) == NULL) {
    sc_warn("Cannot open endpoint protocol %s, reading from stdin", endptprot);
  }
  local_tree = st_tree_init((st_tree *)malloc(sizeof(st_tree)));
  yyparse(local_tree);
  fclose(yyin);

  if (local_tree != NULL) {
#ifdef __DEBUG__
    sc_debug("INIT.1", "Endpoint protocol parsed:");
    st_tree_print(local_tree);
#endif
  } else {
    sc_error("Unable to parse endpoint protocol %s", endptprot);
    return SC_ERR_PROTNVAL; // Protocol invalid
  }

  switch (local_tree->info->type) {
    case ST_TYPE_GLOBAL:
      sc_warn("%s:%d: Endpoint protocol expected but got global protocol, treating as endpoint protocol\n", __FILE__, __LINE__);
    case ST_TYPE_LOCAL:
      _sess->myindex = -1;
      break;
    case ST_TYPE_PARAMETRISED:
      _sess->myindex = 0; // UNASSIGNED
      break;
  }


  //
  // Phase 2: Send local identification to ROOT.
  //

  int *recv_disps = calloc(sizeof(int), nproc);
  int *recv_cnts = calloc(sizeof(int), nproc);
  int i;

  // First send how long are the roles
  int send_buf = strlen(local_tree->info->myrole);
  MPI_Gather(&send_buf, 1, MPI_INT, recv_cnts, 1, MPI_INT, 0, MPI_COMM_WORLD);

  int recvbuf_size = 0;
  for (i=0; i<nproc; ++i) {
    recv_disps[i] = recvbuf_size;
    recvbuf_size += recv_cnts[i];

    sc_debug("INIT.2", "Receive displacement[%d] = %d", i, recv_disps[i]);
    sc_debug("INIT.2", "Receive count[%d] = %d", i, recv_cnts[i]);
  }

  char *recvbuf = calloc(sizeof(char), recvbuf_size);
  sc_debug("INIT.2", "Allocated %d bytes for recvbuf", recvbuf_size);

  assert(strlen(local_tree->info->myrole) < 256);
  MPI_Gatherv(local_tree->info->myrole, strlen(local_tree->info->myrole), MPI_CHAR, recvbuf, recv_cnts, recv_disps, MPI_CHAR, 0, MPI_COMM_WORLD);

  char **rank_role = (char **)calloc(sizeof(char *), nproc);
  for (i=0; i<nproc; ++i) {
    rank_role[i] = calloc(sizeof(char), recv_cnts[i]+1);
    memcpy(rank_role[i], &recvbuf[recv_disps[i]], recv_cnts[i]);
  }
  free(recv_disps);
  free(recv_cnts);
  free(recvbuf);

#ifdef __DEBUG__
  if (0 == procid) {
    sc_debug("ROOT.ROLETBL", "+------+----------\n");
    sc_debug("ROOT.ROLETBL", "| Rank | Role\n");
    sc_debug("ROOT.ROLETBL", "+------+----------\n");
    for (i=0; i<nproc; ++i) {
      sc_debug("ROOT.ROLETBL", "| %4d | %s\n", i, rank_role[i]);
    }
    sc_debug("ROOT.ROLETBL", "+------+----------\n");
  }
#endif

  //
  // Phase 3: Aggregate identification and global protocol to get address table (root only).
  //

  if (0 == procid) {

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
          sc_debug("INIT.3", "Using global protocol file %s", protocol_file);
          break;
      }
    }

    *argc -= optind-1;
    (*argv)[optind-1] = (*argv)[0];
    *argv += optind-1;

    if (protocol_file == NULL) {
      protocol_file = "Protocol.spr";
    }

    sc_debug("INIT.3", "Global protocol file to use: %s", protocol_file);

    if ((yyin = fopen(protocol_file, "r")) == NULL) {
      sc_error("Cannot open global protocol %s", protocol_file);
      return SC_ERR_PROTNVAL; // Global protocol is empty
    }
    global_tree = st_tree_init((st_tree *)malloc(sizeof(st_tree)));
    yyparse(global_tree);
    fclose(yyin);

    if (global_tree != NULL) {
#ifdef __DEBUG__
      sc_debug("INIT.3", "Global protocol parsed:");
      st_tree_print(global_tree);
#endif
    } else {
      sc_error("Unable to parse global protocol %s", protocol_file);
      return SC_ERR_PROTNVAL; // Protocol invalid
    }

    // TODO: Use the global protocol to check if the roles are correct

    // Modify same roles until they are individually identifyable
    int j, k;
    int current_index = -1;
    for (i=0; i<nproc; ++i) {
      current_index = -1;
      for (j=i+1; j<nproc; ++j) {
        if (0 == strcmp(rank_role[i], rank_role[j])) {
          sc_debug("INFO", "Parametrised role found at rank %d & %d (%s)", i, j, rank_role[i]);
          if (current_index == -1) { // We are just starting to count
            // This is for rank_role[j]
            current_index = 2;
            // The global protocol defines what index to start counting
            for (k=0; k<global_tree->info->nrole; ++k) {
              if (0 == strcmp(global_tree->info->roles[k]->name, rank_role[i])) {
                if (NULL != global_tree->info->roles[k]->param
                    && ST_EXPR_TYPE_RANGE == global_tree->info->roles[k]->param->type) {
                  assert(ST_EXPR_TYPE_CONST == global_tree->info->roles[k]->param->binexpr->left->type);
                  current_index = global_tree->info->roles[k]->param->binexpr->left->constant+1; // rank_role[j] is the second index
                  break;
                }
              }
            }

          } else {
            current_index ++;
          }
          rank_role[j] = realloc(rank_role[j], sizeof(char) * (strlen(rank_role[j])+2 + current_index == 0 ? 1 : (int)log10((double)current_index)+1));
          sprintf(rank_role[j], "%s[%d]", rank_role[i], current_index);
        }
      }

      // We found parametrised roles. Now set the first role index.
      if (current_index != -1) {
        current_index = 1; // Default start counting by 1
        // The global protocol defines what index to start counting
        for (k=0; k<global_tree->info->nrole; ++k) {
          if (0 == strcmp(global_tree->info->roles[k]->name, rank_role[i])) {
            if (NULL != global_tree->info->roles[k]->param
                && ST_EXPR_TYPE_RANGE == global_tree->info->roles[k]->param->type) {
              assert(ST_EXPR_TYPE_CONST == global_tree->info->roles[k]->param->binexpr->left->type);
              current_index = global_tree->info->roles[k]->param->binexpr->left->constant; // first index
              break;
            }
          }
        }

        rank_role[i] = realloc(rank_role[i], sizeof(char) * (strlen(rank_role[i])+2 + (current_index == 0 ? 1 : (int)log10((double)current_index)+1)));
        sprintf(rank_role[i], "%s[%d]", rank_role[i], current_index);
      }
    }

#ifdef __DEBUG__
    // Only root
    fprintf(stderr, "+------+----------\n");
    fprintf(stderr, "| Rank | Role\n");
    fprintf(stderr, "+------+----------\n");
    for (i=0; i<nproc; ++i) {
      fprintf(stderr, "| %4d | %s\n", i, rank_role[i]);
    }
    fprintf(stderr, "+------+----------\n");
#endif

  } // if 0==procid

  // Phase 4: Now broadcast the result to all.
  //   1. Displacements
  //   2. Lengths
  //   3. Serialised data
  //
  int *send_disps = calloc(sizeof(int), nproc);
  int *send_cnts = calloc(sizeof(int), nproc);
  char *sendbuf;
  int sendbuf_size = 0;
  if (0 == procid) { // data serialising happens at root only
    for (i=0; i<nproc; ++i) {
      send_disps[i] = sendbuf_size;
      send_cnts[i] = strlen(rank_role[i]);
      sendbuf_size += send_cnts[i];
      if (i==0) {
        sendbuf = (char *)malloc(sizeof(char) * sendbuf_size); // No NULL
      } else {
        sendbuf = (char *)realloc(sendbuf, sizeof(char) * sendbuf_size); // No NULL
      }
      memcpy(&sendbuf[send_disps[i]], rank_role[i], send_cnts[i]);

      sc_debug("INIT.4", "Send displacement[%d] = %d", i, send_disps[i]);
      sc_debug("INIT.4", "Send count[%d] = %d", i, send_cnts[i]);
    }
    sc_debug("INIT.4", "Allocated %d bytes for sendbuf", sendbuf_size);
  }

  MPI_Bcast(send_disps, nproc, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(send_cnts, nproc, MPI_INT, 0, MPI_COMM_WORLD);

  if (0 != procid) {
    sendbuf_size = 0;
    for (i=0; i<nproc; ++i) {
      sendbuf_size += send_cnts[i];
    }
    sendbuf = malloc(sizeof(char) * sendbuf_size); // No NULL
  }

  MPI_Bcast(sendbuf, sendbuf_size, MPI_CHAR, 0, MPI_COMM_WORLD);

  for (i=0; i<nproc; ++i) {
    rank_role[i] = calloc(sizeof(char), send_cnts[i]+1);
    memcpy(rank_role[i], &sendbuf[send_disps[i]], send_cnts[i]);
  }

  free(send_disps);
  free(send_cnts);
  free(sendbuf);

#ifdef __DEBUG__
  sc_debug("INIT.4", "+------+----------");
  sc_debug("INIT.4", "| Rank | Role");
  sc_debug("INIT.4", "+------+----------");
  for (i=0; i<nproc; ++i) {
    sc_debug("INIT.4", "| %4d | %s", i, rank_role[i]);
  }
  sc_debug("INIT.4", "+------+----------");
#endif

  //
  // Phase 5: Store role-rank information to local session
  //

  _sess->nrole = nproc;
  _sess->roles = (role **)calloc(sizeof(role *), nproc);
  for (i=0; i<nproc; ++i) {
    _sess->roles[i] = (role *)malloc(sizeof(st_role_t));
    _sess->roles[i]->type = SESSION_ROLE_P2P;
    _sess->roles[i]->s = _sess;

    _sess->roles[i]->p2p = (struct role_endpoint *)malloc(sizeof(struct role_endpoint));
    _sess->roles[i]->p2p->name = strdup(rank_role[i]);
    char *namedup = strdup(rank_role[i]);
    _sess->roles[i]->p2p->basename = strdup(strtok(namedup, "["));
    if (NULL == _sess->roles[i]->p2p->basename) {
      _sess->roles[i]->p2p->basename = _sess->roles[i]->p2p->name;
    }
    free(namedup);
    _sess->roles[i]->p2p->comm = MPI_COMM_WORLD;
    _sess->roles[i]->p2p->rank = i;

    sc_debug("INIT.5", "roles[%d] = %s(%s)", _sess->roles[i]->p2p->rank, _sess->roles[i]->p2p->name, _sess->roles[i]->p2p->basename);
  }
  _sess->name = strdup(_sess->roles[procid]->p2p->name);
  _sess->_myrank = procid;
  if (_sess->myindex >= 0) { // I am a parametrised role
    char *namedup = strdup(_sess->name);
    sc_debug("INIT.5", "Parametrised role: %s", namedup);
    _sess->basename = strtok(namedup, "[");
    if (NULL == _sess->basename) {
      _sess->basename = _sess->name;
      sc_warn("INIT.5", "Cannot extract basename from parametrised P2P role %s", _sess->name);
    }
    _sess->myindex = atoi(strtok(NULL, "]"));
    sc_debug("INIT.5", "Parametrised role index = %d", _sess->myindex);
    /*
    MPI_Group *_grp = (MPI_Group *)malloc(sizeof(MPI_Group));
    MPI_Comm_group(MPI_COMM_WORLD, _grp);
    sc_debug("INIT.5", "Creating MPI Group %s", _sess->basename);
    for (i=0; i<_sess->nrole; ++i) {
      if (SESSION_ROLE_P2P == _sess->roles[i]->type
          && 0 == strncmp(_sess->basename, _sess->roles[i]->p2p->name, strlen(_sess->basename))) {
        MPI_Group *_newgrp = (MPI_Group *)malloc(sizeof(MPI_Group));
        MPI_Group_excl(*_grp, 1, (int[]){ _sess->roles[i]->p2p->rank }, _newgrp);
        sc_debug("INIT.5", "Group building: exclude %s(%d)", _sess->roles[i]->p2p->name, _sess->roles[i]->p2p->rank);
        free(_grp); // XXX memory leak
        _grp = _newgrp;
      }
      sc_debug("INIT.5", "Group +role %d", i);
    }
    sc_debug("INIT.5", "Group is built, setting creating group role");

    _sess->roles = (role **)realloc(_sess->roles, sizeof(role *) * (_sess->nrole+1));
    _sess->roles[_sess->nrole] = (role *)malloc(sizeof(role));
    _sess->roles[_sess->nrole]->grp->name = strdup(_sess->basename);
    _sess->roles[_sess->nrole]->grp->group = *_grp;
    MPI_Comm_create(MPI_COMM_WORLD, *_grp, &_sess->roles[_sess->nrole]->grp->comm);
    _sess->nrole++;
    */
  }
  free(rank_role);

  free(local_tree);
  if (0 == procid) {
    free(global_tree);
  }

  //
  // Phase 6: Set up request table to track asynchronous requests/buffer management
  //

  sc_req_tbl = (sc_req_tbl_t *)calloc(sizeof(sc_req_tbl_t), _sess->nrole);

  int rank_idx;
  for (rank_idx=0; rank_idx<_sess->nrole; ++rank_idx) {
    sc_req_tbl[rank_idx].nsend = 0;
  }

  _sess->role  = &session__role;
  _sess->rolei = &session__rolei;
  _sess->idx   = &session__idx;
  _sess->coord = &session__coord;
  _sess->param = &session__param;

  // Let's trigger some caching..
  session__role(_sess, "__CACHEHACK__");

#ifdef __DEBUG__
  DEBUG_sess_start_time = sc_time();
#endif
  sc_debug("SESS.INIT", "Complete");
  return SC_SUCCESS;
}


void session_rolegrp_add(session *s, char *name, int root, MPI_Comm comm, MPI_Group group)
{
  sc_debug("SESSGRP", "%s entry: (name=%s, root=%d)", __FUNCTION__, name, root);
  s->roles = (role **)realloc(s->roles, sizeof(role *) * (s->nrole+1));
  s->roles[s->nrole] = (role *)malloc(sizeof(st_role_t));
  s->roles[s->nrole]->type = SESSION_ROLE_GRP;
  s->roles[s->nrole]->s = s;

  s->roles[s->nrole]->grp = (struct role_group *)malloc(sizeof(struct role_group));
  s->roles[s->nrole]->grp->name = strdup(name);
  s->roles[s->nrole]->grp->comm = comm;
  s->roles[s->nrole]->grp->root = root;
  s->roles[s->nrole]->grp->group = group;
  s->nrole++;
  sc_req_tbl = (sc_req_tbl_t *)realloc(sc_req_tbl, sizeof(sc_req_tbl_t) * (s->nrole+1));

  int rank_idx;
  for (rank_idx=0; rank_idx<s->nrole; ++rank_idx) {
    sc_req_tbl[rank_idx].nsend = 0;
  }
  sc_debug("rolegrp_add", "Name: %s Root: %d", name, root);
}


void session_role_add(session *s, char *name, int rank, MPI_Comm comm)
{
  s->roles = (role **)realloc(s->roles, sizeof(role *) * (s->nrole+1));
  s->roles[s->nrole] = (role *)malloc(sizeof(st_role_t));
  s->roles[s->nrole]->type = SESSION_ROLE_P2P;
  s->roles[s->nrole]->s = s;

  s->roles[s->nrole]->p2p = (struct role_endpoint *)malloc(sizeof(struct role_endpoint));
  s->roles[s->nrole]->p2p->name = strdup(name);
  char *namedup = strdup(name);
  s->roles[s->nrole]->p2p->basename = strdup(strtok(namedup, "["));
  if (NULL == s->roles[s->nrole]->p2p->basename) {
    s->roles[s->nrole]->p2p->basename = s->roles[s->nrole]->p2p->name;
  }
  free(namedup);
  s->roles[s->nrole]->p2p->comm = comm;
  s->roles[s->nrole]->p2p->rank = rank;
  s->nrole++;

  sc_req_tbl = (sc_req_tbl_t *)realloc(sc_req_tbl, sizeof(sc_req_tbl_t) * (s->nrole+1));

  int rank_idx;
  for (rank_idx=0; rank_idx<s->nrole; ++rank_idx) {
    sc_req_tbl[rank_idx].nsend = 0;
  }
  sc_debug("role_add", "Name: %s Rank: %d", name, rank);
}


void session_end(session *s)
{
  sc_debug("INFO", "%s entry", __FUNCTION__);
  int role_idx;
  for (role_idx=0; role_idx<s->nrole; ++role_idx) {
    sc_debug("SESS_END", "role_idx=%d nsend=%d", role_idx, sc_req_tbl[role_idx].nsend);
    if (s->roles[role_idx]->type == SESSION_ROLE_P2P && sc_req_tbl[role_idx].nsend > 0) {
      MPI_Waitall(sc_req_tbl[s->roles[role_idx]->p2p->rank].nsend, sc_req_tbl[s->roles[role_idx]->p2p->rank].send_reqs, MPI_STATUSES_IGNORE);
      int i;
      for (i=0; i<sc_req_tbl[ s->roles[role_idx]->p2p->rank ].nsend; ++i) {
        free(sc_req_tbl[ s->roles[role_idx]->p2p->rank ].send_bufs[i]);
      }
      free(sc_req_tbl[ s->roles[role_idx]->p2p->rank ].send_bufs);
      free(sc_req_tbl[ s->roles[role_idx]->p2p->rank ].send_reqs);
      sc_req_tbl[ s->roles[role_idx]->p2p->rank ].nsend = 0;
    }
  }
  free(sc_req_tbl);

#ifdef __DEBUG__
  DEBUG_sess_end_time = sc_time();
#endif

  for (role_idx=0; role_idx<s->nrole; role_idx++) {
    free(s->roles[role_idx]);
  }
  free(s->roles);
  s->nrole = 0;

  free(s);

#ifdef __DEBUG__
  DEBUG_prog_end_time = sc_time();
  sc_debug("STATS", "----- Statistics -----");
  sc_debug("STATS", "Total execution time (including session init and cleanup): %f sec", sc_time_diff(DEBUG_prog_start_time, DEBUG_prog_end_time));
  sc_debug("STATS", "Total time in session: %f sec", sc_time_diff(DEBUG_sess_start_time, DEBUG_sess_end_time));
  sc_debug("STATS", "----------------------");
  sc_debug("INFO", "%s exit", __FUNCTION__);
#endif
  MPI_Finalize();
}


void session_dump(const session *s)
{
  sc_debug("INFO", "%s entry", __FUNCTION__);
  assert(s != NULL);
  unsigned int endpoint_idx;
  unsigned int endpoint_count = s->nrole;

  fprintf(stdout, "\n------Session-------\n");
  fprintf(stdout, "My role: %s (rank %d)\n", s->name, s->_myrank);
  fprintf(stdout, "Number of endpoint roles: %u\n", s->nrole);

  for (endpoint_idx=0; endpoint_idx<endpoint_count; endpoint_idx++) {
    switch (s->roles[endpoint_idx]->type) {
      case SESSION_ROLE_P2P:
        assert(s->roles[endpoint_idx]->p2p != NULL);
        fprintf(stdout, "Endpoint#%u { type: p2p, name: %s, rank: %d }\n",
          endpoint_idx,
          s->roles[endpoint_idx]->p2p->name,
          s->roles[endpoint_idx]->p2p->rank);
        break;
      case SESSION_ROLE_GRP:
        assert(s->roles[endpoint_idx]->grp != NULL);
        fprintf(stdout, "Endpoint#%u { type: group, name: %s, root: %d }\n",
          endpoint_idx,
          s->roles[endpoint_idx]->grp->name,
          s->roles[endpoint_idx]->grp->root);
        break;
      default:
        fprintf(stdout, "Endpoint#%u { type: unknown }\n", endpoint_idx);
    }
  }
  fprintf(stdout, "--------------------\n");
  sc_debug("INFO", "%s exit", __FUNCTION__);
}


// Debug dump
void session_ddump(const session *s)
{
  sc_debug("INFO", "%s entry", __FUNCTION__);
  assert(s != NULL);
  unsigned int endpoint_idx;
  unsigned int endpoint_count = s->nrole;
  int comm_size;

  sc_debug("SESS.DUMP", "------Session-------");
  sc_debug("SESS.DUMP", "My role: %s (rank %d)", s->name, s->_myrank);
  sc_debug("SESS.DUMP", "Number of endpoint roles: %u", s->nrole);

  for (endpoint_idx=0; endpoint_idx<endpoint_count; endpoint_idx++) {
    switch (s->roles[endpoint_idx]->type) {
      case SESSION_ROLE_P2P:
        assert(s->roles[endpoint_idx]->p2p != NULL);
        sc_debug("SESS.DUMP", "Endpoint#%u { type: p2p, name: %s, rank: %d }",
          endpoint_idx,
          s->roles[endpoint_idx]->p2p->name,
          s->roles[endpoint_idx]->p2p->rank);
        break;
      case SESSION_ROLE_GRP:
        assert(s->roles[endpoint_idx]->grp != NULL);
        //MPI_Comm_size(s->roles[endpoint_idx]->grp->comm, &comm_size);
        sc_debug("SESS.DUMP", "Endpoint#%u { type: group, name: %s, root: %d, size:  }",
          endpoint_idx,
          s->roles[endpoint_idx]->grp->name,
          s->roles[endpoint_idx]->grp->root);
         // comm_size);
        break;
      default:
        sc_error("Endpoint#%u { type: unknown }", endpoint_idx);
    }
  }
  sc_debug("SESS.DUMP", "--------------------");
  sc_debug("INFO", "%s exit", __FUNCTION__);
}
