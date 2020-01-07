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
#if defined(HAVE_TOPOLOGY) && defined(HAVE_ATOMICS) && defined(ENABLE_HLS)
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
#else
void * __extls_get_addr_node(tls_index* idx)
{
	UNUSED(idx);
	extls_fatal("HLS at NODE level seems to be used here, but libextls has been build without HLS support. Please install with --enable-hls !");
	return NULL;
}
void * __extls_get_addr_numa_level_2(tls_index* idx)
{
	UNUSED(idx);
	extls_fatal("HLS at NUMA_2 level seems to be used here, but libextls has been built without HLS support. Please install with --enable-hls !");
	return NULL;
}
void * __extls_get_addr_numa_level_1(tls_index* idx)
{
	UNUSED(idx);
	extls_fatal("HLS at NUMA_1 level seems to be used here, but libextls has been built without HLS support. Please install with --enable-hls !");
	return NULL;
}
void * __extls_get_addr_socket(tls_index* idx)
{
	UNUSED(idx);
	extls_fatal("HLS at SOCKET level seems to be used here, but libextls has been built without HLS support. Please install with --enable-hls !");
	return NULL;
}
void * __extls_get_addr_cache_level_3(tls_index* idx)
{
	UNUSED(idx);
	extls_fatal("HLS at CACHE_3 level seems to be used here, but libextls has been built without HLS support. Please install with --enable-hls !");
	return NULL;
}
void * __extls_get_addr_cache_level_2(tls_index* idx)
{
	UNUSED(idx);
	extls_fatal("HLS at CACHE_2 level seems to be used here, but libextls has been built without HLS support. Please install with --enable-hls !");
	return NULL;
}
void * __extls_get_addr_cache_level_1(tls_index* idx)
{
	UNUSED(idx);
	extls_fatal("HLS at CACHE_1 level seems to be used here, but libextls has been built without HLS support. Please install with --enable-hls !");
	return NULL;
}
void * __extls_get_addr_core(tls_index* idx)
{
	UNUSED(idx);
	extls_fatal("HLS at CORE level seems to be used here, but libextls has been built without HLS support. Please install with --enable-hls !");
	return NULL;
}
#endif


#undef dlopen
#undef dlsym
#undef dlclose

/**
 * this function is a wrapper between the user call and the real libc call.
 *
 * It's the runtime's responsability to ensure the user dl* calls are properly wrapped by our extls_dl* routines
 * This is a really important point, especially if the loaded object
 * contains a TLS segment.
 *
 * In our case, the goal of dlopen() wrapper is to detect and register the potential new TLS segment.
 * For more specific information about parameters and dlopen(), please refer yourself to "man dlopen"
 *
 * \param filename the path to the object as a string
 * \param flag
 * \return a handler generated by dlopen()
 */
void * extls_dlopen(const char* filename, int flag)
{
	/* here, it has to be the real dlopen() ! */
	void * handler = dlopen(filename, flag);

	extls_register_dyn_tls_segment(filename, handler);

	return handler;
}

/**
 * this function is a wrapper between the user call and the read libc call.
 *
 * It's the runtime's responsability to ensure the user dl* calls are properly wrapped by our extls_dl* routines
 * This is a really important point, especially if the loaded object contains a TLS segment.
 *
 * In our case, the call of dlsym() wrapper is to create a copy for the required symbol, if it is a TLS.
 * If so, we have to elect it to the appropriate level. By default, the dlsym() wrapper elects the symbol
 * at thread level (__thread equivalent). If you prefer allocating the TLS symbol at a different level, please
 * consider the use of extls_ldlsym() (the "l" for level).
 * \see extls_ldlsym
 *
 * For more information about parameters and dlsym() behavior, please refer yourself to "man dlsym"
 * \param handle the library handler returned by dlopen() (the real one)
 * \param symbol the symbol name to look for, as a string
 *
 * \return a pointer on the TLS, allocated in the specific level.
 */
void * extls_dlsym(void *handle, const char*symbol)
{
	/* simple request for a thread level TLS */
	return extls_ldlsym(handle, symbol, LEVEL_THREAD);
}

/**
 * this function handles a dlsym() request by providing a pointer on that TLS but depending on the level
 * the TLS need to be manipulated.
 *
 * This function is called by the dlsym() wrapper but can also be called directly.
 * In any way, if the symbol is not a TLS, the dlsym() is simply called and the result in simply forwarded.
 *
 * \param handle the dlopen() handler
 * \param symbol the symbol name, the same provided to the real dlsym()
 * \param level the desired level to elect the TLS (if exists)
 *
 * \return
 * <ul>
 * <li>a pointer on a pre-allocated region for the given TLS level. The pointer is valid only in our library
 * addressing space.</li>
 * <li> a pointer returned by dlsym() if the symbol is not a TLS. Please refer to the man for return values</li>
 * <li>NULL otherwise </li>
 * </ul>
 */
inline void * extls_ldlsym(void* handle, const char*symbol, extls_object_level_type_t level)
{
	void * ret = NULL;
	extls_size_t module_id = 0, symbol_offset = 0;

	/* We only handle (for now) dynamics symbol lookup with the dlopen'd handler */
	if(handle == RTLD_NEXT || handle == RTLD_DEFAULT)
	{
		extls_fatal("The library does not handle RTLD_NEXT nor RTLD_DEFAULT for now.");
	}

	/* If the dynamically loaded library does not contains a TLS segment OR the handle
	 * provided is not referenced in our base, just forward the call to the real dlsym()
	 */
	if(extls_get_dyn_tls_segmt_index(&module_id, handle) == EXTLS_ENFOUND)
	{
		goto fallback_real_dlsym;
	}

	/* First, need to compute the offset inside the module
	 * We have to check if the symbol is a TLS symbol.
	 * If not, the symbol has to be parsed by real dlsym()
	 */
	if(extls_get_dyn_tls_offset(module_id, symbol, &symbol_offset) == EXTLS_ENFOUND)
	{
		goto fallback_real_dlsym;
	}

	/* retrieve the current tls vector (current calling thread) */
	extls_object_level_t* tls_vec = (*((extls_ctx_t**)extls_get_context_storage_addr()))->tls_vector;

	/* sanity check */
	if(tls_vec == NULL)
	{
		extls_fatal("Pointer retrieved by extls_get_context_storage_addr() should not be null !");
	}

	extls_object_level_t* tls_level = &(tls_vec[level]);
	extls_size_t dyn_module = module_id - extls_get_nb_static_tls_segments(); /* index in dyn_seg */

	/* We allocates a static array. This simplify ctx heritage between threads.
	 * However it's not possible to dynamically realocate the array (no indirection).
	 * We can (for now) support up to 8 concurrent dlopen'd modules (Shared object w/ TLS segment)
	 */
	if((*tls_level->nb_dyn_seg) >= EXTLS_DEFAULT_DYN_SEGMENTS)
	{
		extls_fatal("The library does not handle more than %d dlopen'd modules !", EXTLS_DEFAULT_DYN_SEGMENTS);
	}

	/* allocate the segment the first time */
	if(tls_level->dyn_seg[dyn_module] == UNALLOCATED_MODULE)
	{
		extls_map_dynamic_segment(tls_level, dyn_module);
	}

	ret = (char*)tls_level->dyn_seg[dyn_module] + symbol_offset;

	goto ret_func;

/* label pointers, depending on symbol lookup */
fallback_real_dlsym:
	ret = dlsym(handle, symbol);

ret_func:
	return ret;
}

/**
 * This function is a simple wrapper around dlclose() function.
 * It is the runtime's responsibility this function is called in place of the real dlclose().
 *
 * This function simply free potential TLS segments created by dlsym().
 *
 * \param handle the TLS handler returned by dlsym()
 *
 * \return 0 if succeeded. Please refer to dlclose man-pages for further information.
 */
int extls_dlclose(void * handle)
{
	int ret;
	extls_size_t module_id = 0;

	/* if we don't handle this handler, it means it does not contain a TLS segment */
	if(extls_get_dyn_tls_segmt_index(&module_id, handle) == EXTLS_ENFOUND)
	{
		goto ret_func;
	}

	/* retrieve the current tls vector (current calling thread) */
	extls_size_t nb_static_segments = extls_get_nb_static_tls_segments();
	extls_size_t dyn_module = module_id - nb_static_segments;
	extls_object_level_t* tls_vec = (*((extls_ctx_t**)extls_get_context_storage_addr()))->tls_vector;

	/* sanity check */
	if(tls_vec == NULL)
	{
		extls_fatal("Pointer retrieved by extls_get_context_storage_addr() should not be null !");
	}

	/* free potential malloc'd segment, for any level */
	int i = 0;
	for(i=0; i < LEVEL_MAX; i++)
	{
		extls_object_level_t* tls_level = &(tls_vec[i]);


		if(tls_level->dyn_seg[dyn_module] != NULL)
		{
			extls_size_t sz = extls_get_seg_size_for(dyn_module + nb_static_segments);
			if(sz >= (extls_size_t)getpagesize())
			{
				munmap(tls_level->dyn_seg[dyn_module], sz);
			}
			else
			{
				free(tls_level->dyn_seg[dyn_module]);
			}
			tls_level->dyn_seg[dyn_module] = NULL;
		}
	}

ret_func:
	ret = dlclose(handle);
	return ret;
}
