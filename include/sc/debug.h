#ifndef SC__DEBUG_H__
#define SC__DEBUG_H__
/**
 * \file
 * Session C runtime library MPI backend (libsc-mpi)
 * debugging and error reporting module.
 */

#ifdef __DEBUG__
#define sc_debug(...) __sc_debug(__VA_ARGS__)
#else
#define sc_debug(...) do {} while(0)
#endif

#include <stdarg.h>

void __sc_debug(char *tag, const char *format, ...);
void sc_error(const char *format, ...);
void sc_warn(const char *format, ...);

#endif // SC__DEBUG_H__
