#ifndef SC__PRIMITIVES_MPI_H__
#define SC__PRIMITIVES_MPI_H__
/**
 * \file
 * Session C runtime library MPI backend (libsc-mpi)
 * communication primitives module.
 */

#include <stdarg.h>

#include "sc/types-mpi.h"

#define SC_REQ_MGR_SEND 0
#define SC_REQ_MGR_RECV 1

/**
 * \brief Send an integer.
 *
 * @param[in] val   Value to send
 * @param[in] r     Role to send to
 * @param[in] label Message label
 *
 * \returns 0 if successful, -1 otherwise and set errno
 *          (See man page of zmq_send)
 */
int send_int(int val, role *r, int label);


/**
 * \brief Send an integer.
 *
 * @param[in] arr   Array to send
 * @param[in] count Number of elements in array
 * @param[in] r     Role to send to
 * @param[in] label Message label
 *
 * \returns 0 if successful, -1 otherwise and set errno
 *          (See man page of zmq_send)
 */
int send_int_array(const int arr[], size_t count, role *r, int label);


/**
 * \brief Send an integer to multiple roles.
 *
 * @param[in] val         Value to send
 * @param[in] nr_of_roles Number of roles to send to 
 * @param[in] ...         Variable number (subject to nr_of_roles)
 *                        of role variables
 *
 * \returns 0 if successful, -1 otherwise and set errno
 *          (See man page of zmq_recv)
 */
int vsend_int(int val, int nr_of_roles, ...);


/**
 * \brief Receive an integer (pre-allocated).
 *
 * @param[out] dst Pointer to variable storing recevied value
 * @param[in]  r   Role to receive from
 *
 * \returns 0 if successful, -1 otherwise and set errno
 *          (See man page of zmq_recv)
 */
int recv_int(int *dst, role *r);


/**
 * \brief Receive an integer array (pre-allocated).
 *
 * @param[out]    arr   Pointer to array storing recevied value
 * @param[in,out] count Number of elements in array
 * @param[in]     r     Role to receive from
 *
 * \returns 0 if successful, -1 otherwise and set errno
 *          (See man page of zmq_recv)
 */
int recv_int_array(int *arr, size_t count, role *r);


/**
 * \breif Broadcast an integer.
 *
 * @param[in] val   Value to send
 * @param[in] s     Session to broadcast to
 * 
 * \returns 0 if successful, -1 otherwise and set errno
 *          (See man page of zmq_send)
 */
int bcast_int(int val, session *s);


/**
 * \breif Broadcast an integer array.
 *
 * @param[in] arr   Array to send
 * @param[in] count Number of elements in array
 * @param[in] s     Session to broadcast to
 * 
 * \returns 0 if successful, -1 otherwise and set errno
 *          (See man page of zmq_send)
 */
int bcast_int_array(const int arr[], size_t count, session *s);


/**
 * \brief Receive a broadcast integer.
 *
 * @param[out]    dst   Pointer to variable storing recevied value
 * @param[in]     s     Session to receive broadcast from
 *
 * \returns 0 if successful, -1 otherwise and set errno
 *          (See man page of zmq_send)
 */
int brecv_int(int *dst, session *s);


/**
 * \brief Receive a broadcast integer array (pre-allocated).
 *
 * @param[out]    arr   Pointer to array storing recevied value
 * @param[in,out] count Pointer to variable storing number of elements in array
 * @param[in]     s     Session to receive broadcast from
 *
 * \returns 0 if successful, -1 otherwise and set errno
 *          (See man page of zmq_send)
 */
int brecv_int_array(int *arr, size_t *count, session *s);


/**
 * \brief Barrier synchronisation.
 *
 * @param[in] grp_role    Group role to perform barrier synchronisation on
 * @param[in] at_rolename Role name (string) to act as central coordinator
 *
 * \returns 0 if successful, -1 otherwise and set errno
 *          (See man page of zmq_recv)
 */
int barrier(role *grp_role, char *at_rolename);

#endif // SC__PRIMITIVES_MPI_H_
