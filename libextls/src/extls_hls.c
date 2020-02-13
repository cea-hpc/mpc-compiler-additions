#include <config.h>
#include <sched.h>
#include "extls_common.h"
#include "extls_atomics.h"
#include "extls_topo.h"
#include "extls_hls.h"
#include "extls_segmt_hdler.h"

#if defined(HAVE_TOPOLOGY) && defined(ENABLE_HLS)
extern FILE* fd; /**< In extls.c */
extern extls_lock_t fd_lock; /**< In extls.c */
extern char extls_own_topology; /**< In extls_common.c */
static short hls_initialized = 0; /**< check HLS init in case of remote topology */

static inline extls_ret_t extls_hls_data_init(extls_hls_data_t** data)
{
	*data = malloc(sizeof(extls_hls_data_t));
	extls_atomic_store(&(*data)->nb_toenter,0);
	extls_atomic_store(&(*data)->nb_entered,0);
	extls_atomic_store(&(*data)->async_gen_num,0);
	(*data)->sync_gen_num = 0;
	return EXTLS_SUCCESS;
}

static inline const char* extls_get_obj_name(extls_topo_obj_t obj)
{
	switch(obj->type)
	{
		case HWLOC_OBJ_MACHINE:
			return "MACHINE"; break;
		case HWLOC_OBJ_NODE:
			return "NUMA"; break;
		case HWLOC_OBJ_SOCKET:
			return "SOCKET"; break;
		case HWLOC_OBJ_CACHE:
			return "CACHE"; break;
		case HWLOC_OBJ_CORE:
			return "CORE"; break;
		case HWLOC_OBJ_PU:
			return "PU"; break;
		case HWLOC_OBJ_GROUP:
			return "GROUP"; break;
		default:
			return "NOT_HANDLED"; break;
	}
	return NULL;
}

static inline int extls_obj_hls_handled(extls_topo_obj_t obj)
{
	switch(obj->type)
	{
		case HWLOC_OBJ_MACHINE:
		case HWLOC_OBJ_NODE:
		case HWLOC_OBJ_SOCKET:
		case HWLOC_OBJ_CACHE:
		case HWLOC_OBJ_CORE:
		case HWLOC_OBJ_PU:
		case HWLOC_OBJ_GROUP:
			return 1;
		default:
			return 0;
	}
	return -1;
}
static extls_topo_obj_t extls_hls_set_with_first_ancestor(extls_topo_t* topology, extls_topo_obj_t obj, extls_obj_type_t type, extls_object_level_t* level, extls_object_level_type_t set)
{
	extls_topo_obj_t ret = obj;
	extls_topo_obj_t tmp = extls_parent_by_type(*topology,type, obj);
	
	if(tmp == NULL)
	{
		if(set <= LEVEL_NODE)
		{
			extls_fatal("The desired level %s is required and not present in the topology !", LEVEL_NAME(set));
		}
		else
		{
			level[set].static_seg = NULL; /* set to NULL to identify it as "unresolved" */
		}
	}
	else
	{
		level[set] = *((extls_object_level_t*)((uintptr_t)strtoull(extls_obj_get_info(tmp, "object_level"), NULL, 16)));
		extls_atomic_incr(&level[set].hls_data->nb_toenter);
		ret = tmp;
	}
	return ret;
}

extls_ret_t extls_hls_init_levels(extls_object_level_t* start_array, int pu)
{

	if(start_array[LEVEL_NODE].static_seg != NULL)
		return EXTLS_SUCCESS;

	/* We have to browse through the HLS levels only !!!! */
	extls_topo_t* topology = extls_get_topology_addr();

	/* This toplogy object can be NULL if the runtime does provide its own topology object (like MPC)
	 * and thus defer the call to initialize the topology (call to extls_hls_topology_construct().
	 * For that specific case, we don't want program to crash (even if the behavior should be undefined)
	 * In order to do this, the library will "emulate" a temp toplogy, and build the current context on it.
	 * (this may be the one created by extls_init() before anything has started up yet.
	 */
	if(!topology || !(*topology)) /* if _construct() haven't been called yet */
	{
		/* use the internal topology for now, will be replaced once runtime decided to do so*/
		topology = extls_get_own_topology_addr();
        	extls_hls_topology_construct();
        	hls_initialized = 0; /* this is ugly, this boolean need to be set to 1 when the real topology is initialized */
	}
	else if(!hls_initialized)
	{
        	extls_fatal("HLS: extls_hls_topology_construct() should be called before running the first context !");
        }


	extls_topo_obj_t cur_level = extls_obj_by_type(*topology, HWLOC_OBJ_PU, pu);
	if(cur_level == NULL)
		extls_fatal("We cannot found the deepest topology object for the current binding %d", pu);
	
	/* cur_level is updated w/ the found object */
	cur_level = extls_hls_set_with_first_ancestor(topology, cur_level, HWLOC_OBJ_CORE,    start_array, LEVEL_CORE);
	cur_level = extls_hls_set_with_first_ancestor(topology, cur_level, HWLOC_OBJ_CACHE,   start_array, LEVEL_CACHE_1);
	cur_level = extls_hls_set_with_first_ancestor(topology, cur_level, HWLOC_OBJ_CACHE,   start_array, LEVEL_CACHE_2);
	cur_level = extls_hls_set_with_first_ancestor(topology, cur_level, HWLOC_OBJ_CACHE,   start_array, LEVEL_CACHE_3);

	/* we do not update cur_level, to avoid squashing current ref */
	extls_hls_set_with_first_ancestor(topology, cur_level, HWLOC_OBJ_SOCKET,  start_array, LEVEL_SOCKET);	
	cur_level = extls_hls_set_with_first_ancestor(topology, cur_level, HWLOC_OBJ_NODE,    start_array, LEVEL_NUMA_1);

	/* idem as above */
	extls_hls_set_with_first_ancestor(topology, cur_level, HWLOC_OBJ_NODE,    start_array, LEVEL_NUMA_2);
	
	cur_level = extls_hls_set_with_first_ancestor(topology, cur_level, HWLOC_OBJ_MACHINE, start_array, LEVEL_NODE);

	extls_object_level_type_t i = 0;
	for(i = LEVEL_NODE; i < LEVEL_MAX; i++)
	{
		if(start_array[i].static_seg == NULL)
			start_array[i] = start_array[i-1]; /* The first level (NODE) will not pass through this statement (extls_fatal()) */
	}

	/* big print to generate topology file */
	PRINT_TOPOLOGY("CTX_%p [shape=\"box\" color=\"green\" label=\"CTX bound to %d\\nNODE=%p\\lNUMA2=%p\\lNUMA1=%p\\lSOCKET=%p\\lCACHE3=%p\\lCACHE2=%p\\lCACHE1=%p\\lCORE=%p\\l\"]\n", start_array, pu, start_array[LEVEL_NODE].static_seg, start_array[LEVEL_NUMA_2].static_seg, start_array[LEVEL_NUMA_1].static_seg, start_array[LEVEL_SOCKET].static_seg, start_array[LEVEL_CACHE_3].static_seg, start_array[LEVEL_CACHE_2].static_seg, start_array[LEVEL_CACHE_1].static_seg, start_array[LEVEL_CORE].static_seg);
	PRINT_TOPOLOGY("N%p -> CTX_%p [color=\"green\"]\n", start_array[LEVEL_CORE].static_seg, start_array);

	return EXTLS_SUCCESS;
}
#define str(u) #u
extls_ret_t extls_hls_topology_construct(void)
{
	if(hls_initialized)
	{
        	extls_warn("HLS tree should be initialized only once !");
        	return EXTLS_ENFIRST;
	}

	extls_topo_t* global_topology = extls_get_topology_addr();
	if(!global_topology || !(*global_topology))
	{
		global_topology = extls_get_own_topology_addr();
	}
	/* get global tree depth an number of NUMA nodes */
	const int topodepth = extls_topology_depth(*global_topology);
	extls_topo_obj_t* stack ;
	int stack_idx;

	stack = alloca( (topodepth+1) * sizeof(extls_topo_obj_t) );
	stack_idx = 0 ;
	stack[stack_idx]   = extls_root_obj (*global_topology) ;
	stack[stack_idx+1] = NULL ;
	extls_info("HLS: Registering Global Objects");
	do
	{
		assert(stack_idx < topodepth+1 ) ;
		extls_topo_obj_t cur_obj    = stack[stack_idx] ;
		extls_topo_obj_t prev_child = stack[stack_idx+1] ;
		extls_topo_obj_t next_child = extls_next_child(*global_topology,cur_obj,prev_child);

		if ( prev_child == NULL && extls_obj_hls_handled(cur_obj))
		{
			char str[16];

			extls_object_level_t* level = malloc(sizeof(extls_object_level_t));
			extls_map_static_segment(level);
			level->dyn_seg = calloc(EXTLS_DEFAULT_DYN_SEGMENTS, sizeof(extls_object_t*));
			level->nb_dyn_seg = malloc(sizeof(extls_size_t));
			*(level->nb_dyn_seg) = 0;
			level->dyn_lock = malloc(sizeof(extls_lock_t));
			extls_lock_init(level->dyn_lock, NULL);
			extls_hls_data_init(&level->hls_data);

			sprintf ( str, "%p", level ) ;

			/* only if hwloc tree dumping has been requested (export EXTLS_DUMP_TOPOLOGY)" */
			PRINT_TOPOLOGY("N%p [label=\"%s #%d\\n%p\", color=\"red\"]\n", level->static_seg, extls_get_obj_name(cur_obj), cur_obj->logical_index, level->static_seg);
			if(cur_obj->parent != NULL)
				PRINT_TOPOLOGY("N%p -> N%p\n [color=\"cyan\"]\n",((extls_object_level_t*)(uintptr_t)strtoull(extls_obj_get_info(cur_obj->parent, "object_level"), NULL, 16))->static_seg , level->static_seg);
			extls_obj_add_info ( cur_obj, "object_level", str) ;
		}

		if ( next_child != NULL && next_child->type != HWLOC_OBJ_PU )
		{
			stack_idx += 1 ;
			stack[stack_idx] = next_child ;
			stack[stack_idx+1] = NULL ;
		}
		else
		{
			stack_idx -= 1 ;
		}
	}
	while ( stack_idx >= 0 ) ;
    hls_initialized = 1;
	return EXTLS_SUCCESS;
}

extls_ret_t extls_hls_topology_init(void)
{
	/* here is the first call to extls_get_topology_addr() 
	 * This is a weak function, only called if the user code did not
	 * overloaded it. In that particular case, the library is notified
	 * through the use of `extls_own_topology` that it should take care
	 * of init and destroy the underlying topology object.
	 */
	extls_topo_t* t = extls_get_topology_addr();
	if(extls_own_topology)
	{
		extls_topology_init(t);
		extls_topology_load(*t);
		return extls_hls_topology_construct();
	}

	return EXTLS_ENFIRST;
}

extls_ret_t extls_hls_topology_fini(void)
{
	extls_topo_t* t = extls_get_topology_addr();
	if(extls_own_topology)
		extls_topology_destroy(*t);
	return EXTLS_SUCCESS;
}

void __extls_hls_barrier(unsigned int hls_level)
{
	extls_object_level_type_t level = hls_level + LEVEL_NODE;
	extls_ctx_t* ctx = *((extls_ctx_t**)extls_get_context_storage_addr());
	if(ctx->pu == -1) /* if ctx is not bound, abort... */
	{
		extls_fatal("Unable to call HLS routines if the context is not bound !");
	}

	extls_dbg("BARRIER AT LEVEL %s", LEVEL_NAME(level));
	extls_hls_data_t* hls_struct = ctx->tls_vector[level].hls_data;

	/* mark the current generation as resolved */
	ctx->sync_gen_num++;

	/* mark the HLS level to notify a new ctx has reached the barrier */
	const int nb_cur_entered = extls_atomic_fetch_and_incr(&hls_struct->nb_entered);
	
	/* if the current ctx is the last arrived */
	if(nb_cur_entered + 1 == extls_atomic_load(&hls_struct->nb_toenter))
	{
		/* re-init the level */
		extls_atomic_store(&hls_struct->nb_entered, 0);
		/* ensure the store() are done in-order */
		extls_atomic_write_barrier();
		/* increment the resolved generation counter */
		hls_struct->sync_gen_num = ctx->sync_gen_num;
	}
	else /* it is not the last arrived... */
	{
		/* Wait for the last context reached the current generation */ 
		extls_wait_for_value(&hls_struct->sync_gen_num, ctx->sync_gen_num);
	}

}

int __extls_hls_single(unsigned int hls_level)
{
	extls_object_level_type_t level = hls_level + LEVEL_NODE;
	extls_ctx_t* ctx = *((extls_ctx_t**)extls_get_context_storage_addr());
	if(ctx->pu == -1) /* if the ctx is not bound, abort...*/
	{
		extls_fatal("Unable to call HLS routines if the context is not bound !");
	}

	extls_dbg("SINGLE AT LEVEL %s", LEVEL_NAME(level));
	extls_hls_data_t* hls_struct = ctx->tls_vector[level].hls_data;

	/* the current thread has reached the current generation */
	ctx->sync_gen_num++;

	/* notify the global level that a new ctx has reached the current generation */
	const int nb_cur_entered = extls_atomic_fetch_and_incr(&hls_struct->nb_entered);
	
	/* if the current thread is the last for the current generation,
	 * he is elected to run the critical section.
	 * (Maybe would be better to let the FIRST one to execute the critical section)
	 */
	if(nb_cur_entered + 1 == extls_atomic_load(&hls_struct->nb_toenter))
	{
		return 1;
	}
	else /* if it is not the last ctx... */
	{
		/* Wait for the last context to reach the current generation */ 
		extls_wait_for_value(&hls_struct->sync_gen_num, ctx->sync_gen_num);
		return 0;
	}
}

void __extls_hls_single_done(unsigned int hls_level)
{
	extls_object_level_type_t level = hls_level + LEVEL_NODE;
	extls_ctx_t* ctx = *((extls_ctx_t**)extls_get_context_storage_addr());
	if(ctx->pu == -1) /* if the ctx is not bound, abort... */
	{
		extls_fatal("Unable to call HLS routines if the context is not bound !");
	}
	extls_dbg("SINGLE DONE AT LEVEL %s", LEVEL_NAME(level));
	extls_hls_data_t* hls_struct = ctx->tls_vector[level].hls_data;

	/* routine only called by the thread executing the critical section */
	/* Here, we have to reset the current level and set the number 
	 * of reached genereation, to unlock waiting threads
	 */
	extls_atomic_store(&hls_struct->nb_entered, 0);
	/* ensure in-order write calls */
	extls_atomic_write_barrier();
	/* unlock the waiting threads */
	hls_struct->sync_gen_num = ctx->sync_gen_num;
}

int __extls_hls_single_nowait(unsigned int hls_level)
{
	int ret = 0;
	extls_object_level_type_t level = hls_level + LEVEL_NODE;
	extls_ctx_t* ctx = *((extls_ctx_t**)extls_get_context_storage_addr());
	if(ctx->pu == -1) /* if the current cts is not bound, abort... */
	{
		extls_fatal("Unable to call HLS routines if the context is not bound !");
	}
	extls_dbg("SINGLE NOWAIT AT LEVEL %s", LEVEL_NAME(hls_level + LEVEL_NODE));
	extls_hls_data_t* hls_struct = ctx->tls_vector[level].hls_data;

	/* get how many nowait have been encountered for the current level */
	const int level_generation = extls_atomic_load(&hls_struct->async_gen_num);
	/* if the ctx is the first to reach the nowait (otherwise, level_generation
	 * should be higher than the currently reached ref counter
	 */
	if(level_generation <= ctx->async_gen_num)
	{
		/* if an another ctx steals the nowait, we ensure the critical
		 * section with an atomic "compare and swap" and a return value check.
		 * ret contains 1 if the "CAS" succeeded for this level, 0 otherwise
		 */
		ret = extls_atomic_cmp_and_swap(&hls_struct->async_gen_num, level_generation, level_generation+1);
		ret = (ret == level_generation);
	}
	/* finally, we can jump directly to the "last retrieved" value for the
	 * global generation counter. Obviously, we know that every "single" between
	 * our counter and the global ones will not be processed by this ctx
	 */
	ctx->async_gen_num++;

	return ret;

}

#else /* WITHOUT HLS SUPPORT */

void __extls_hls_barrier(unsigned int hls_level)
{
	UNUSED(hls_level);
	extls_fatal("A HLS BARRIER routine seems to be inserted but libextls has been built without HLS support. Please install with --enable-hls !");
}

int __extls_hls_single(unsigned int hls_level)
{
	UNUSED(hls_level);
	extls_fatal("A HLS SINGLE routine seems to be inserted but libextls has been built without HLS support. Please install with --enable-hls !");
	return 0;
}

int __extls_hls_single_nowait(unsigned int hls_level)
{
	UNUSED(hls_level);
	extls_fatal("A HLS SINGLE_NOWAIT routine seems to be inserted but libextls has been built without HLS support. Please install with --enable-hls !");
	return 0;
}

void __extls_hls_single_done(unsigned int hls_level)
{
	UNUSED(hls_level);
	extls_fatal("A HLS SINGLE_DONE routine seems to be inserted but libextls has been built without HLS support. Please install with --enable-hls !");
}

#endif

