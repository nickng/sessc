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
 * \brief Send integer.
 *
 * @param[in] vals  Values to send
 * @param[in] count Number of elements to send
 * @param[in] r     Role to send to
 * @param[in] label Message label
 *
 * \returns SC_SUCCESS (==MPI_SUCCESS) if successful.
 */
int send_int(int *vals, int count, role *r, int label);


/**
 * \brief Receive integer (pre-allocated).
 *
 * @param[out] vals        Pointer to variable storing recevied value
 * @param[in]  count       Number of elements to receive
 * @param[in]  r           Role to receive from
 * @param[in]  match_label Message label to match
 *
 * \returns SC_SUCCESS (==MPI_SUCCESS) if successful.
 */
int recv_int(int *vals, int count, role *r, int match_label);


/**
 * \brief All-Broadcast integer.
 *
 * @param[in,out] vals  Value to send
 * @param[in]     count Number of elements to send/receive
 * @param[in]     g     Group role to broadcast to
 * 
 * \returns SC_SUCCESS (==MPI_SUCCESS) if successful.
 */
int alltoall_int(int *vals, int count, role *g);


/**
 * \brief Send double.
 *
 * @param[in] vals  Values to send
 * @param[in] count Number of elements to send
 * @param[in] r     Role to send to
 * @param[in] label Message label
 *
 * \returns SC_SUCCESS (==MPI_SUCCESS) if successful.
 */
int send_double(double *vals, int count, role *r, int label);


/**
 * \brief Receive double (pre-allocated).
 *
 * @param[out] vals        Pointer to variable storing recevied value
 * @param[in]  count       Number of elements to receive
 * @param[in]  r           Role to receive from
 * @param[in]  match_label Message label to match
 *
 * \returns SC_SUCCESS (==MPI_SUCCESS) if successful.
 */
int recv_double(double *vals, int count, role *r, int match_label);


/**
 * \brief All-Broadcast integer.
 *
 * @param[in,out] vals  Value to send
 * @param[in]     count Number of elements to send/receive
 * @param[in]     g     Group role to broadcast to
 * 
 * \returns SC_SUCCESS (==MPI_SUCCESS) if successful.
 */
int alltoall_double(double *vals, int count, role *g);


/**
 * \brief Barrier synchronisation.
 *
 * @param[in] s Session to perform barrier synchronisation on
 *
 * \returns SC_SUCCESS (==MPI_SUCCESS) if successful.
 */
int barrier(session *s, int buffersync);

#endif // SC__PRIMITIVES_MPI_H_
