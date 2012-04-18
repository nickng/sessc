#ifndef __CONNMGR_H__
#define __CONNMGR_H__
/**
 * \file
 * Header file for connection manager component.
 *
 */

#define MAX_NR_OF_ROLES 100 // Maximum number of endpoint roles.
#define MAX_HOSTNAME_LENGTH 256

#define CONNMGR_TYPE_P2P 1
#define CONNMGR_TYPE_GRP 2

// A connection record.
typedef struct {
  int type;
  char *from;
  char *to;
  char *host;
  unsigned port;
} conn_rec;

// A role-host map.
typedef struct {
  char *role;
  char *host;
} host_map;

/**
 * \brief Load a hosts file.
 *
 * @param[in]  hostsfile Hosts file path
 * @param[out] hosts     Array to hold list of hosts
 *
 * \returns Number of hosts loaded.
 */
int connmgr_load_hosts(const char *hostfile, char ***hosts);


/**
 * \brief Load roles in session from file.
 *
 * @param[in]  scribble global Scribble file path
 * @param[out] roles    Array to hold list of roles
 *
 * \returns Number of roles loaded.
 */
int connmgr_load_roles(const char *scribble, char ***roles);


/**
 * \brief Create a connection record array using given parameters.
 *
 * @param[out] conns       Connection record array
 * @param[out] role_hosts  Role-to-host mapping
 * @param[in]  roles       Roles array
 * @param[in]  roles_count Number of items in roles array
 * @param[in]  hosts       Hosts array
 * @param[in]  hosts_count Number of items in hosts array
 * @param[in[  start_port  Lowest port number used in the connectoin records
 * 
 * \returns Number of items in connection record array.
 */
int connmgr_init(conn_rec **conns, host_map **role_hosts,
                 char **roles, int roles_count,
                 char **hosts, int hosts_count,
                 int start_port);


/**
 * \brief Read a connection record file.
 *
 * @param[in]  infile      Input file path
 * @param[out] conns       Connection record array to write to
 * @param[out] role_hosts  Role-to-host mapping
 * @param[out] nr_of_roles Number of roles 
 *
 * \returns Number of items in the connection record array.
 */
int connmgr_read(const char *infile, conn_rec **conns, host_map **role_hosts, int *nr_of_roles);


/**
 * \brief Write a connection record array to file.
 *
 * @param[in] outfile     Output file path
 * @param[in] conns       Connection record array
 * @param[in] nr_of_conns Number of items in connection record array
 * @param[in] role_hosts  Role-to-host mapping
 * @param[in] nr_of_roles Number of roles in connection record
 */
void connmgr_write(const char *outfile, const conn_rec conns[], int nr_of_conns,
                                        const host_map role_hosts[], int nr_of_roles);

#endif // __CONNMGR_H__
