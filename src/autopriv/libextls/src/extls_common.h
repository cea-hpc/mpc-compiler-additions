#ifndef HAVE_EXTLS_COMMON_H
#define HAVE_EXTLS_COMMON_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <assert.h>
#include "extls_types.h"

/** defined in extls.c */
extern const char * const object_level_name[];

/** get the level value in a readable format */
#define LEVEL_NAME(u) object_level_name[u]
/** consider TLS blocks as "not initialized yet" */
#define UNALLOCATED_MODULE NULL
/** Max number of concurrently loaded dynamic segment supported by the library */
#define EXTLS_DEFAULT_DYN_SEGMENTS 8
/** round a to the next multiple of b */
#define roundup(a, b) (( ( a + (b-1) ) / b ) * b )

/* set macro depending on colouring support */
#ifdef EXTLS_COLOR_ENABLED
#define BLU "\033[1;36m"
#define RED "\033[1;31m"
#define YEL "\033[1;33m"
#define GRE "\033[1;32m"
#define DEF "\033[0;0m"
#else
#define BLU ""
#define RED ""
#define YEL ""
#define GRE ""
#define DEF ""
#endif

/** macro to remove warnings when a function parameter is not used but have to be a parameter */
#ifndef UNUSED
#define UNUSED(a) (void)sizeof(a)
#endif
#define PRINT_TOPOLOGY(str,...) do{if(fd != NULL){extls_lock(&fd_lock); fprintf(fd, str, ##__VA_ARGS__);extls_unlock(&fd_lock);}}while(0)

#define extls_not_impl() do{extls_warn("Function %s() not implemented Yet !"DEF, __func__); }while(0)

#ifndef NDEBUG
/* different routines to print information at different verbosity levels */
#define extls_dbg(u,...)   do{if(extls_get_verbosity() >= EXTLS_VERB_DEBUG)fprintf(stderr, BLU "EXTLS-DEBUG: " u DEF" (%s():"RED"%d"DEF")\n", ##__VA_ARGS__, __FUNCTION__, __LINE__);}while(0)
#define extls_info(u,...)  do{if(extls_get_verbosity() >= EXTLS_VERB_INFO)fprintf(stderr, GRE "EXTLS-INFO : " u DEF"\n", ##__VA_ARGS__);}while(0)
#define extls_warn(u,...)  do{if(extls_get_verbosity() >= EXTLS_VERB_WARN)fprintf(stderr, YEL "EXTLS-WARN : " u DEF"\n", ##__VA_ARGS__);}while(0)
#define extls_err(u,...) do{if(extls_get_verbosity() >= EXTLS_VERB_ERROR)fprintf(stderr, RED "EXTLS-ERROR: " u DEF"\n", ##__VA_ARGS__);}while(0)
#define extls_fatal(u,...) do{if(extls_get_verbosity() >= EXTLS_VERB_ERROR)fprintf(stderr, RED "EXTLS-FATAL: " u DEF"\n", ##__VA_ARGS__);abort();}while(0)
#define extls_assert(u) do{if(!u) extls_fatal("Assertion " #u " returned false.");} while(0)

#else
#define extls_dbg(u,...)  (void)(u)
#define extls_info(u,...) (void)(u)
#define extls_warn(u,...)  do{fprintf(stderr, YEL "EXTLS-WARN : " u DEF"\n", ##__VA_ARGS__);}while(0)
#define extls_err extls_fatal
#define extls_fatal(u,...) do{fprintf(stderr, RED "EXTLS-FATAL: " u DEF"\n", ##__VA_ARGS__);abort();}while(0)
#define extls_assert(u) (void)(u)
#endif

/* return code interpreter */
void extls_strerror_r(extls_ret_t type, char* str, size_t sz);
char *extls_strerror(extls_ret_t type);

/* misc */
extls_ret_t extls_write_zeros(int fd, extls_size_t sz);

/* verbosity */
void extls_init_verbosity(void);
extls_verb_t extls_get_verbosity(void);
void extls_set_verbosity(extls_verb_t set);

///void* extls_get_dflt_context_storage_addr(void);
//extls_topo_t* extls_get_dflt_topology_addr(void);
void extls_wait_for_value(volatile int*, int);

extls_ret_t extls_fallback_ctx_init();
extls_ret_t extls_fallback_ctx_reset();
extls_ctx_t* extls_fallback_ctx_get();

#ifdef __cplusplus
}
#endif
#endif
