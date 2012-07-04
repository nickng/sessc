/**
 * \file
 * Session C runtime library (libsc)
 * parametrised utilities module.
 *
 * Supports tools for writing programs
 * with parametrised roles.
 */

#include <assert.h>

#include "st_node.h"

#include "sc/parametrised.h"
#include "sc/types.h"


int sc_role_index(const session *s)
{
  if (s->is_parametrised) {
    return s->index;
  }

  return -1;
}
