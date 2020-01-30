#ifndef HAVE_EXTLS_TOPO_H
#define HAVE_EXTLS_TOPO_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef HAVE_TOPOLOGY
#include <hwloc.h>

/* create an interface between hwloc and extls */
typedef hwloc_topology_t extls_topo_t;
typedef hwloc_obj_t      extls_topo_obj_t;
typedef hwloc_obj_type_t extls_obj_type_t;

#define extls_topology_init    hwloc_topology_init
#define extls_topology_load    hwloc_topology_load
#define extls_topology_destroy hwloc_topology_destroy
#define extls_topology_depth   hwloc_topology_get_depth

#define extls_nbobjs_by_type   hwloc_get_nbobjs_by_type
#define extls_obj_by_type      hwloc_get_obj_by_type

#define extls_root_obj         hwloc_get_root_obj
#define extls_next_child	   hwloc_get_next_child
#define extls_parent_by_type   hwloc_get_ancestor_obj_by_type
#define extls_obj_add_info     hwloc_obj_add_info
#define extls_obj_get_info     hwloc_obj_get_info_by_name
#define extls_type_depth       hwloc_get_type_depth
#define extls_parent_by_depth  hwloc_get_ancestor_obj_by_depth
#else
typedef void* extls_topo_t;
typedef void* extls_topo_obj_t;

#endif

#ifdef __cplusplus
}
#endif
#endif
