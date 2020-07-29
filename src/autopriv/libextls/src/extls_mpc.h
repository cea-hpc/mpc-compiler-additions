#ifndef EXTLS_MPC_H
#define EXTLS_MPC_H


#ifdef __cplusplus
extern "C"
{
#endif

#include "extls_types.h"
#include "extls_optim_tls.h"

/* really specific to TLS handling in Intel compiler, bound to MPC callbacks */

/* TLS routines */
void * __sctk__tls_get_addr__process_scope(tls_index* idx);
void * __sctk__tls_get_addr__task_scope(tls_index* idx);
void * __sctk__tls_get_addr__thread_scope(tls_index* idx);
void * __sctk__tls_get_addr__openmp_scope(tls_index* idx);

/* HLS routines */
void * __sctk__tls_get_addr__node_scope(tls_index* idx);
void * __sctk__tls_get_addr__numa_level_2_scope(tls_index* idx);
void * __sctk__tls_get_addr__numa_level_1_scope(tls_index* idx);
void * __sctk__tls_get_addr__socket_scope(tls_index* idx);
void * __sctk__tls_get_addr__cache_level_3_scope(tls_index* idx);
void * __sctk__tls_get_addr__cache_level_2_scope(tls_index* idx);
void * __sctk__tls_get_addr__cache_level_1_scope(tls_index* idx);
void * __sctk__tls_get_addr__core_scope(tls_index* idx);

#ifdef __cplusplus
}
#endif
#endif
