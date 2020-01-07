#ifndef HAVE_EXTLS_HLS_H
#define HAVE_EXTLS_HLS_H

extls_ret_t extls_hls_topology_init(void);
extls_ret_t extls_hls_topology_fini(void);
extls_ret_t extls_hls_topology_construct(void);
extls_ret_t extls_hls_init_levels(extls_object_level_t* start_array, int pu);
#endif
