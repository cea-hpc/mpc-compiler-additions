#include "extls_mpc.h"

/* TLS routines */
void * __sctk__tls_get_addr__process_scope(tls_index* idx)
{
	return __extls_get_addr_process(idx);
}

void * __sctk__tls_get_addr__task_scope(tls_index* idx)
{
	return __extls_get_addr_task(idx);
}
void * __sctk__tls_get_addr__thread_scope(tls_index* idx)
{
	return __extls_get_addr_thread(idx);
}
void * __sctk__tls_get_addr__openmp_scope(tls_index* idx)
{
	return __extls_get_addr_openmp(idx);
}

/* HLS routines */
#if defined(HAVE_TOPOLOGY) && defined(HAVE_ATOMICS) && defined(ENABLE_HLS)
void * __sctk__tls_get_addr__node_scope(tls_index* idx)
{
	return __extls_get_addr_node(idx);
}
void * __sctk__tls_get_addr__numa_level_2_scope(tls_index* idx)
{
	return __extls_get_addr_numa_level_2(idx);
}
void * __sctk__tls_get_addr__numa_level_1_scope(tls_index* idx)
{
	return __extls_get_addr_numa_level_1(idx);
}
void * __sctk__tls_get_addr__socket_scope(tls_index* idx)
{
	return __extls_get_addr_socket(idx);
}
void * __sctk__tls_get_addr__cache_level_3_scope(tls_index* idx)
{
	return __extls_get_addr_cache_level_3(idx);
}
void * __sctk__tls_get_addr__cache_level_2_scope(tls_index* idx)
{
	return __extls_get_addr_cache_level_2(idx);
}
void * __sctk__tls_get_addr__cache_level_1_scope(tls_index* idx)
{
	return __extls_get_addr_cache_level_1(idx);
}
void * __sctk__tls_get_addr__core_scope(tls_index* idx)
{
	return __extls_get_addr_core(idx);
}
#endif
