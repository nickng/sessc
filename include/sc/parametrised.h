#ifndef SC__PARAMETRISED_H__
#define SC__PARAMETRISED_H__
/**
 * \file
 * Session C runtime library (libsc)
 * parametrised utilities module.
 */

#include "sc/types.h"

/**
 * \brief Get the role index of the current running process.
 *
 * @param[in] s Session which the role belongs
 *
 * \returns -1 if not parametrised role,
 *          role index otherwise.
 */
int sc_role_index(const session *s);


#endif // SC__PARAMETRISED_H__
