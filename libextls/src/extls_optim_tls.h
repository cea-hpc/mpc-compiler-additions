#ifndef HAVE_EXTLS_OPTIM_TLS_H
#define HAVE_EXTLS_OPTIM_TLS_H

#include "extls_common.h"

/* Used by GNU variant */
struct tls_index_s
{
	unsigned long int ti_module;
	unsigned long int ti_offset;
};
typedef struct tls_index_s tls_index;


extls_ret_t extls_optim_tls_set_pointer(extls_object_t start);
extls_ret_t extls_optim_tls_check_method();

extls_size_t extls_get_offset_for(extls_size_t module_id);
extls_size_t extls_get_seg_size_for(extls_size_t module_id);
extls_ret_t extls_register_dyn_tls_segment(const char* filename, void * handler);
extls_ret_t extls_get_dyn_tls_segmt_index(extls_size_t *idx, void* handler);
extls_ret_t extls_get_dyn_tls_offset(extls_size_t idx, const char * sym, extls_size_t* off_out);


/* TLS */
void * __extls_get_addr_process(tls_index* idx);
void * __extls_get_addr_task(tls_index* idx);
void * __extls_get_addr_thread(tls_index* idx);
void * __extls_get_addr_openmp(tls_index* idx);

/* HLS */
#if defined(HAVE_TOPOLOGY) && defined(HAVE_ATOMICS) && defined(ENABLE_HLS)
void * __extls_get_addr_node(tls_index* idx);
void * __extls_get_addr_numa_level_2(tls_index* idx);
void * __extls_get_addr_numa_level_1(tls_index* idx);
void * __extls_get_addr_socket(tls_index* idx);
void * __extls_get_addr_cache_level_3(tls_index* idx);
void * __extls_get_addr_cache_level_2(tls_index* idx);
void * __extls_get_addr_cache_level_1(tls_index* idx);
void * __extls_get_addr_core(tls_index* idx);
#endif

#endif
