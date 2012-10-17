/**
 * \file
 * Session C runtime library MPI backend (libsc-mpi)
 * debugging and error reporting module.
 */

#include <stdio.h>
#include <stdarg.h>

#include <mpi.h>

#include "sc/debug.h"

static FILE *DEBUG_FILE = NULL;


/**
 * Fake function for debugging. See sc/debug.h for sc_debug macro.
 *
 * @param[in] tag    Tag to go with the debug message for filtering
 * @param[in] format printf-format string
 * @param[in] ...    printf argument
 */
void __sc_debug(char *tag, const char *format, ...)
{
#ifdef __DEBUG__
  if (NULL == DEBUG_FILE) DEBUG_FILE = stderr;

  int rank;
  if (MPI_SUCCESS == MPI_Comm_rank(MPI_COMM_WORLD, &rank)) {
    fprintf(DEBUG_FILE, "[\033[0;34m#%s\033[0m/\033[32m%d\033[0m] ", tag, rank);

    va_list argptr;
    va_start(argptr, format);
    vfprintf(DEBUG_FILE, format, argptr);
    va_end(argptr);
    fprintf(DEBUG_FILE, "\n");
    fflush(DEBUG_FILE);
  }
#endif
}


void sc_error(const char *format, ...)
{
  if (NULL == DEBUG_FILE) DEBUG_FILE = stderr;

  int rank;
  if (MPI_SUCCESS == MPI_Comm_rank(MPI_COMM_WORLD, &rank)) {
    fprintf(DEBUG_FILE, "[\033[1;31mERROR\033[32m/\033[32m%d\033[0m] ", rank);
  }

  va_list argptr;
  va_start(argptr, format);
  vfprintf(DEBUG_FILE, format, argptr);
  va_end(argptr);
  fprintf(DEBUG_FILE, "\n");
  fflush(DEBUG_FILE);
}


void sc_warn(const char *format, ...)
{
  if (NULL == DEBUG_FILE) DEBUG_FILE = stderr;

  int rank;
  if (MPI_SUCCESS == MPI_Comm_rank(MPI_COMM_WORLD, &rank)) {
    fprintf(DEBUG_FILE, "[\033[0;31mWARN\033[0m/\033[32m%d\033[0m] ", rank);
  }

  va_list argptr;
  va_start(argptr, format);
  vfprintf(DEBUG_FILE, format, argptr);
  va_end(argptr);
  fprintf(DEBUG_FILE, "\n");
  fflush(DEBUG_FILE);
}
