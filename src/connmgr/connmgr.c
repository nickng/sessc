/**
 * \file
 * Connection manager component of Session C runtime.
 * The main purpose of the manager is to generate a suitable
 * connection configuration file from given global Scribble desciption.
 *
 */

#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "connmgr.h"
#include "st_node.h"

extern int yyparse();
extern FILE *yyin;


/**
 * Load a hosts file (ie. sequential list of hosts) into memory.
 */
int connmgr_load_hosts(const char *hostsfile, char ***hosts)
{
#ifdef __DEBUG__
  fprintf(stderr, "%s(%s)\n", __FUNCTION__, hostsfile);
#endif
  FILE *hosts_fp;
  int host_idx = 0;
  char buf[MAX_HOSTNAME_LENGTH]; // XXX: Potential buffer overflow.

  *hosts = malloc(sizeof(char *) * MAX_NR_OF_ROLES);

  if ((hosts_fp = fopen(hostsfile, "r")) == NULL) {
    perror(__FUNCTION__);
    return 0;
  }
  while (fscanf(hosts_fp, "%s", buf) != EOF && host_idx < MAX_NR_OF_ROLES) {
    (*hosts)[host_idx] = malloc(sizeof(char) * MAX_HOSTNAME_LENGTH); // XXX
    strncpy((*hosts)[host_idx], buf, MAX_HOSTNAME_LENGTH-1);
#ifdef __DEBUG__
    fprintf(stderr, "%s: host#%d %s\n", __FUNCTION__, host_idx, (*hosts)[host_idx]);
#endif
    host_idx++;
  }
  fclose(hosts_fp);

  return host_idx;
}


/**
 * Load and parse a global Scribble to extract the roles into memory.
 *
 */
int connmgr_load_roles(const char *scribble, char ***roles)
{
#ifdef __DEBUG__
  fprintf(stderr, "%s(%s)\n", __FUNCTION__, scribble);
#endif
  // XXX Size of roles[] is MAX_NR_OF_ROLES
  *roles = malloc(sizeof(char *) * MAX_NR_OF_ROLES);
  st_tree *tree = st_tree_init((st_tree *)malloc(sizeof(st_tree)));
  if ((yyin = fopen(scribble, "r")) == NULL) {
    perror(__FUNCTION__);
    return 0;
  }
  yyparse(tree);
  fclose(yyin);

#ifdef __DEBUG__
  st_tree_print(tree);
#endif


  if (!tree->info->global) {
    fprintf(stderr, "%s:%d Warning: %s(): %s is not a global protocol.\n", __FILE__, __LINE__, __FUNCTION__, scribble);
  }

  int i;
  for (i=0; i<tree->info->nrole; ++i) {
    (*roles)[i] = (char *)calloc(sizeof(char), strlen(tree->info->roles[i])+1);
    strcpy((*roles)[i], tree->info->roles[i]);
  }

  free(tree);
  return i;
}


/**
 * Initialise Connection manager with given roles and hosts list.
 */
int connmgr_init(conn_rec **conns, host_map **role_hosts,
                 char **roles, int roles_count,
                 char **hosts, int hosts_count,
                 int start_port)
{
#ifdef __DEBUG__
  fprintf(stderr, "%s(%d roles, %d hosts)\n", __FUNCTION__, roles_count, hosts_count);
#endif

  if (hosts_count == 0) {
    fprintf(stderr, "%s: Warning: No hosts defined, defaulting to localhost for all processes\n", __FUNCTION__);
    hosts = (char **)malloc(sizeof(char *) * roles_count);
    int host_idx;
    for (host_idx=0; host_idx<roles_count; ++host_idx) {
      hosts[host_idx] = "localhost";
    }
    hosts_count = roles_count;
  }


  // Allocate memory for conn_rec.
  int nr_of_connections = (roles_count*(roles_count-1))/2; // N * (N-1) / 2
  *conns = (conn_rec *)malloc(sizeof(conn_rec) * (nr_of_connections + roles_count));
  conn_rec *cr = *conns; // Alias.

  // Create a map of role to host.
  *role_hosts = (host_map *)malloc(sizeof(host_map) * roles_count);
  host_map *rh = *role_hosts;

  if (roles_count > hosts_count) {
    fprintf(stderr, "Warning: Number of hosts is less than number of roles.");
  }

  int role_idx, host_idx;
  for (role_idx=0; role_idx<roles_count; ++role_idx) {
    rh[role_idx].role = (char *)calloc(sizeof(char), (strlen(roles[role_idx]) + 1));
    strcpy(rh[role_idx].role, roles[role_idx]);

    host_idx = role_idx % hosts_count; // Reuse hosts from beginning
    rh[role_idx].host = (char *)calloc(sizeof(char), (strlen(hosts[host_idx]) + 1));
    strcpy(rh[role_idx].host, hosts[host_idx]);
  }

  unsigned port_nr;
  int conn_idx, conn2_idx, map_idx, role2_idx;
  char *from_host;
  for (role_idx=0, conn_idx=0; role_idx<roles_count; ++role_idx) {
    for (role2_idx=role_idx+1; role2_idx<roles_count; ++role2_idx) {
      assert(conn_idx<nr_of_connections);
      cr[conn_idx].type = CONNMGR_TYPE_P2P;
      // Set from-role.
      cr[conn_idx].from = (char *)calloc(sizeof(char), (strlen(roles[role_idx]) + 1));
      strcpy(cr[conn_idx].from, roles[role_idx]);

      // Set to-role.
      cr[conn_idx].to = (char *)calloc(sizeof(char), (strlen(roles[role2_idx]) + 1));
      strcpy(cr[conn_idx].to, roles[role2_idx]);

      // Lookup host from role_hosts map.
      for (map_idx=0; map_idx<roles_count; ++map_idx) {
        if (strcmp(cr[conn_idx].from, rh[map_idx].role) == 0) {
          from_host = (char *)calloc(sizeof(char), (strlen(rh[map_idx].host) + 1));
          strcpy(from_host, rh[map_idx].host);
        }
        if (strcmp(cr[conn_idx].to, rh[map_idx].role) == 0) {
          cr[conn_idx].host = (char *)calloc(sizeof(char), (strlen(rh[map_idx].host) + 1));
          strcpy(cr[conn_idx].host, rh[map_idx].host);
        }
      }


      if (strcmp(from_host, cr[conn_idx].host) == 0) {
        cr[conn_idx].host = (char *)realloc(cr[conn_idx].host, sizeof(char) * (strlen(cr[conn_idx].host) + 4));
        sprintf(cr[conn_idx].host, "ipc:%s", from_host);
      }
      // Find next unoccupied port.
      port_nr = -1;
      for (conn2_idx=0; conn2_idx<conn_idx; ++conn2_idx) {
        if (strcmp(cr[conn2_idx].host, cr[conn_idx].host) == 0) {
          port_nr = cr[conn2_idx].port;
        }
      }
      cr[conn_idx].port = (port_nr==-1 ? start_port : port_nr+1);

      ++conn_idx;
    }
  }

  // Broadcast role generation
  for (role_idx=0; role_idx<roles_count; ++role_idx) {
    cr[conn_idx].type = CONNMGR_TYPE_GRP;
    cr[conn_idx].from = (char *)calloc(sizeof(char), (strlen(roles[role_idx]) + 1));
    strcpy(cr[conn_idx].from, roles[role_idx]);

    cr[conn_idx].to = (char *)calloc(sizeof(char), (strlen(roles[role_idx]) + 1));
    strcpy(cr[conn_idx].to, roles[role_idx]);

    // Lookup host from role_hosts map.
    for (map_idx=0; map_idx<roles_count; ++map_idx) {
      if (strcmp(cr[conn_idx].to, rh[map_idx].role) == 0) {
        cr[conn_idx].host = (char *)calloc(sizeof(char), (strlen(rh[map_idx].host) + 1));
        strcpy(cr[conn_idx].host, rh[map_idx].host);
      }
    }

    // Find next unoccupied port.
    port_nr = -1;
    for (conn2_idx=0; conn2_idx<conn_idx; ++conn2_idx) {
      if (strcmp(cr[conn2_idx].host, cr[conn_idx].host) == 0) {
        port_nr = cr[conn2_idx].port;
      }
    }
    cr[conn_idx].port = (port_nr==-1 ? start_port : port_nr+1);

    ++conn_idx;
  }

  printf("%d %d %d\n", nr_of_connections, roles_count, conn_idx);

  return nr_of_connections + roles_count;
}


/**
 * Write connection record array to file.
 */
void connmgr_write(const char *outfile, const conn_rec conns[], int nconns,
                                        const host_map role_hosts[], int nroles)
{
  FILE *out_fp;
  int conn_idx, role_idx;
#ifdef __DEBUG__
  fprintf(stderr, "%s(%s, %d connections, %d roles)\n", __FUNCTION__, outfile, nconns, nroles);
#endif

  if (strcmp(outfile, "-") == 0) {
    out_fp = stdout;
  } else {
    out_fp = fopen(outfile, "w");
  }

  fprintf(out_fp, "%d %d\n", nroles, nconns);

  for (role_idx=0; role_idx<nroles; ++role_idx) {
    fprintf(out_fp, "%s %s\n",
              role_hosts[role_idx].role,
              role_hosts[role_idx].host);
  }

  for (conn_idx=0; conn_idx<nconns; ++conn_idx) {
    fprintf(out_fp, "%d %s %s %s %d\n",
              conns[conn_idx].type,
              conns[conn_idx].from,
              conns[conn_idx].to,
              conns[conn_idx].host,
              conns[conn_idx].port);
  }
  if (out_fp != stdout) fclose(out_fp);
}


/**
 * Read from file the connection record array.
 */
int connmgr_read(const char *infile, conn_rec **conns, host_map **role_hosts, int *nr_of_roles)
{
  FILE *in_fp;
  int conn_idx, role_idx;
  int nr_of_conns = 0;

  conn_rec *cr;
  host_map *rh;

  if ((in_fp = fopen(infile, "r")) == NULL) {
    perror(__FUNCTION__);
    return 0;
  }

  fscanf(in_fp, "%d %d", nr_of_roles, &nr_of_conns);
  *conns      = malloc(sizeof(conn_rec) * nr_of_conns);
  *role_hosts = malloc(sizeof(host_map) * (*nr_of_roles));
  cr = *conns;
  rh = *role_hosts;

  for (role_idx=0; role_idx<*nr_of_roles; ++role_idx) {
    rh[role_idx].role = malloc(sizeof(char) * MAX_HOSTNAME_LENGTH);
    rh[role_idx].host = malloc(sizeof(char) * MAX_HOSTNAME_LENGTH);
    fscanf(in_fp, "%s %s", rh[role_idx].role, rh[role_idx].host);
#ifdef __DEBUG__
    fprintf(stderr, "%s: #%d %s %s\n",
                      __FUNCTION__, role_idx, rh[role_idx].role, rh[role_idx].host);
#endif
  }

  for (conn_idx=0; conn_idx<nr_of_conns; ++conn_idx) {
    cr[conn_idx].from = malloc(sizeof(char) * MAX_HOSTNAME_LENGTH);
    cr[conn_idx].to   = malloc(sizeof(char) * MAX_HOSTNAME_LENGTH);
    cr[conn_idx].host = malloc(sizeof(char) * MAX_HOSTNAME_LENGTH);
    fscanf(in_fp, "%d %s %s %s %u\n", &cr[conn_idx].type, cr[conn_idx].from, cr[conn_idx].to, cr[conn_idx].host, &cr[conn_idx].port);
#ifdef __DEBUG__
    fprintf(stderr, "%s: #%d %s->%s %s:%u\n",
                      __FUNCTION__, conn_idx, cr[conn_idx].from, cr[conn_idx].to, cr[conn_idx].host, cr[conn_idx].port);
#endif
  }

  fclose(in_fp);

  return conn_idx;
}

