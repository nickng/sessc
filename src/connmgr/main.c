#include <stdio.h>
#include <stdlib.h>

#include "connmgr.h"

int main(int argc, char **argv)
{
  conn_rec *conns;
  int conns_count;
  host_map *hosts_roles;

  char **hosts;
  int hosts_count;
  char **roles;
  int roles_count;

  if (argc < 4) {
    fprintf(stderr, "Not enough arguments\n");
    fprintf(stderr, "Usage: %s hostfile scribblefile outputfile\n", argv[0]);
    fprintf(stderr, "       use `-' for stdout\n");
    return EXIT_FAILURE;
  }

  printf("Host file: %s\nScribble file: %s\nOutput connection configuration: %s\n", argv[1], argv[2], argv[3]);

  hosts_count = connmgr_load_hosts(argv[1], &hosts);
  roles_count = connmgr_load_roles(argv[2], &roles);

  conns_count = connmgr_init(&conns, &hosts_roles, roles, roles_count, hosts, hosts_count, 6666);
  connmgr_write(argv[3], conns, conns_count, hosts_roles, roles_count);
  return EXIT_SUCCESS;
}
