#include <config.h>
#include "extls_segmt_hdler.h"
#include "extls_optim_tls.h"
#include "extls.h"
#if defined(__x86_64__)
#include <asm/prctl.h>
#include <sys/prctl.h>
//prototype not defined with old libc version
extern int arch_prctl(int, unsigned long);
//#elif
#endif
extern void*(*extls_get_context_storage_addr)(void);

/* Is the user wanting to disable optimized TLS handling ? */
extern short tlsopt_support;

extls_ret_t extls_optim_tls_set_pointer(extls_object_t start)
{

	if(!tlsopt_support)
	{
		return EXTLS_SUCCESS;
	}

/*************************************************************************************************/
#if defined(__x86_64__)
	extls_dbg("Optim TLS: Setting GS with %p", start);
	if(arch_prctl(ARCH_SET_GS, (long)start))
	{
		const int SIZE=128;
		char buf[SIZE];
		extls_fatal("Switching GS register failed: %s", strerror_r(errno, buf, SIZE));
		return EXTLS_ENKNWN;
	}
//#elif
#else
#warning "EXTLS-MACHINE: Architecrure not supported for extls optimized TLS !"
#endif
/*************************************************************************************************/

	return EXTLS_SUCCESS;
}

extls_ret_t extls_optim_tls_check_method()
{
	//standard optimized tls settings
	char input = (char)123;
	char output = (char)0;
	extls_optim_tls_set_pointer((void*)&input);

	if(!tlsopt_support)
	{
		return EXTLS_EARCH;
	}

/*************************************************************************************************/

	//checks depending on architecture
#if defined(__x86_64__)
	__asm__("mov\t%%gs:0x0,%0 \n" : "=r"(output));
//#elif
#else
#warning "EXTLS-MACHINE: Architecture not verified for extls optimized TLS !"
#endif

/*************************************************************************************************/

	if(input != output)
	{
		return EXTLS_EARCH;
	}
	return EXTLS_SUCCESS;
}

void * extls_generic__tls_get_addr(tls_index* idx, extls_object_level_type_t level)
{
	extls_ctx_t* ctx = (*(extls_ctx_t**)extls_get_context_storage_addr());
	extls_object_level_t* vector = ctx->tls_vector;
	extls_object_t address = NULL;

	if(vector == NULL)
	{
		extls_fatal("Pointer retrieved by extls_get_context_storage_addr() should not be null !");
	}

	if(level > LEVEL_TLS_MAX && ctx->pu == -1)
	{
		extls_fatal("Impossible to access HLS through get_addr() if the context is not bound !");
	}

	extls_size_t nb_static_elems = extls_get_nb_static_tls_segments();

	if(idx->ti_module > nb_static_elems)
	{
		extls_dbg("Get Addr(): Access dlopen'd TLS block");
		/* we remove 1 because we index from 0 the dynamic modules */
		extls_size_t dyn_module = (idx->ti_module-1) - nb_static_elems;

		if(vector[level].dyn_seg[dyn_module] == UNALLOCATED_MODULE)
		{
			extls_map_dynamic_segment(&(vector[level]), dyn_module);
		}

		address = (char*)vector[level].dyn_seg[dyn_module] + idx->ti_offset;
	}
	else
	{
		extls_dbg("Get Addr(): Access static TLS block");
		address = (char*)vector[level].static_seg - extls_get_offset_for(idx->ti_module) + idx->ti_offset;
	}

	extls_info("Get Addr(): returns %p", address);
	return address;
}

void * __extls_get_addr_task(tls_index* idx)
{
	extls_info("Get Addr(): "RED"TASK"GRE" request at (0x%x):0x%x", (unsigned int)idx->ti_module, (unsigned int)idx->ti_offset);
	return extls_generic__tls_get_addr(idx, LEVEL_TASK);
}
void * __extls_get_addr_process(tls_index* idx)
{
	extls_info("Get Addr(): "RED"PROCESS"GRE" request at (0x%x):0x%x", (unsigned int)idx->ti_module, (unsigned int)idx->ti_offset);
	return extls_generic__tls_get_addr(idx, LEVEL_PROCESS);
}
void * __extls_get_addr_thread(tls_index* idx)
{
	extls_info("Get Addr(): "RED"THREAD"GRE" request at (0x%x):0x%x", (unsigned int)idx->ti_module, (unsigned int)idx->ti_offset);
	return extls_generic__tls_get_addr(idx, LEVEL_THREAD);
}
void * __extls_get_addr_openmp(tls_index* idx)
{
	extls_info("Get Addr(): "RED"OPENMP"GRE" request at (0x%x):0x%x", (unsigned int)idx->ti_module, (unsigned int)idx->ti_offset);
	return extls_generic__tls_get_addr(idx, LEVEL_OPENMP);
}

/***** HLS ******/
void * __extls_get_addr_node(tls_index* idx)
{
	extls_info("Get Addr(): "RED"NODE"GRE" request at (0x%x):0x%x", (unsigned int)idx->ti_module, (unsigned int)idx->ti_offset);
	return extls_generic__tls_get_addr(idx, LEVEL_NODE);
}
void * __extls_get_addr_numa_level_2(tls_index* idx)
{
	extls_info("Get Addr(): "RED"NUMA 2"GRE" request at (0x%x):0x%x", (unsigned int)idx->ti_module, (unsigned int)idx->ti_offset);
	return extls_generic__tls_get_addr(idx, LEVEL_NUMA_2);
}
void * __extls_get_addr_numa_level_1(tls_index* idx)
{
	extls_info("Get Addr(): "RED"NUMA 1"GRE" request at (0x%x):0x%x", (unsigned int)idx->ti_module, (unsigned int)idx->ti_offset);
	return extls_generic__tls_get_addr(idx, LEVEL_NUMA_1);
}
void * __extls_get_addr_socket(tls_index* idx)
{
	extls_info("Get Addr(): "RED"SOCKET"GRE" request at (0x%x):0x%x", (unsigned int)idx->ti_module, (unsigned int)idx->ti_offset);
	return extls_generic__tls_get_addr(idx, LEVEL_SOCKET);
}
void * __extls_get_addr_cache_level_3(tls_index* idx)
{
	extls_info("Get Addr(): "RED"CACHE 3"GRE" request at (0x%x):0x%x", (unsigned int)idx->ti_module, (unsigned int)idx->ti_offset);
	return extls_generic__tls_get_addr(idx, LEVEL_CACHE_3);
}
void * __extls_get_addr_cache_level_2(tls_index* idx)
{
	extls_info("Get Addr(): "RED"CACHE 2"GRE" request at (0x%x):0x%x", (unsigned int)idx->ti_module, (unsigned int)idx->ti_offset);
	return extls_generic__tls_get_addr(idx, LEVEL_CACHE_2);
}
void * __extls_get_addr_cache_level_1(tls_index* idx)
{
	extls_info("Get Addr(): "RED"CACHE 1"GRE" request at (0x%x):0x%x", (unsigned int)idx->ti_module, (unsigned int)idx->ti_offset);
	return extls_generic__tls_get_addr(idx, LEVEL_CACHE_1);
}
void * __extls_get_addr_core(tls_index* idx)
{
	extls_info("Get Addr(): "RED"CORE"GRE" request at (0x%x):0x%x", (unsigned int)idx->ti_module, (unsigned int)idx->ti_offset);
	return extls_generic__tls_get_addr(idx, LEVEL_CORE);
}