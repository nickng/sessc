#ifndef SC__PRIMITIVES_H__
#define SC__PRIMITIVES_H__
/**
 * \file
 * Session C runtime library (libsc)
 * communication primitives module.
 */

#include <stdarg.h>

#include "sc/types.h"

/**
 * \brief Send an integer.
 *
 * @param[in] val Value to send
 * @param[in] r   Role to send to
 *
 * \returns 0 if successful, -1 otherwise and set errno
 *          (See man page of zmq_send)
 */
int send_int(int val, role *r);


/**
 * \brief Send an integer.
 *
 * @param[in] val   Array to send
 * @param[in] count Number of elements in array
 * @param[in] r     Role to send to
 *
 * \returns 0 if successful, -1 otherwise and set errno
 *          (See man page of zmq_send)
 */
int send_int_array(const int arr[], size_t count, role *r);


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
 * @param[in,out] count Pointer to variable storing number of elements in array
 * @param[in]     r     Role to receive from
 *
 * \returns 0 if successful, -1 otherwise and set errno
 *          (See man page of zmq_recv)
 */
int recv_int_array(int *arr, size_t *count, role *r);


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


#endif // SC__PRIMITIVES_H__
