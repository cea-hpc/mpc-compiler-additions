#ifndef HAVE_EXTLS_HLS_H
#define HAVE_EXTLS_HLS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "extls.h"
#include "extls_types.h"

extls_ret_t extls_hls_topology_init(void);
extls_ret_t extls_hls_topology_fini(void);
extls_ret_t extls_hls_topology_construct(void);
extls_ret_t extls_hls_init_levels(extls_object_level_t* start_array, int pu);
extls_ret_t extls_hls_herit_levels(extls_object_level_t* new, extls_object_level_t* old);

#ifdef __cplusplus
}
#endif

#endif
