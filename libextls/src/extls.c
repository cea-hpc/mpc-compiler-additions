#include <config.h>
#include "extls.h"
#include "extls_optim_tls.h"
#include "extls_segmt_hdler.h"
#include "extls_dynamic.h"

extern extls_topo_t* (*extls_get_topology_addr)(void);
extern void*(*extls_get_context_storage_addr)(void);

#if defined(HAVE_TOPOLOGY) && defined(ENABLE_HLS)
#include "extls_hls.h"
#endif

/** Array used to print a level object in a readable format */
const char * const object_level_name[] = {
	"PROCESS" /* LEVEL_PROCESS */, "TASK   " /* LEVEL_TASK    */,
	"THREAD " /* LEVEL_THREAD  */, "OPENMP " /* LEVEL_OPENMP  */,
	"NODE   " /* LEVEL_NODE    */, "NUMA 1 " /* LEVEL_NUMA_1  */,
	"NUMA 2 " /* LEVEL_NUMA_2  */, "SOCKET " /* LEVEL_SOCKET  */,
	"CACHE 3" /* LEVEL_CACHE_3 */, "CACHE 2" /* LEVEL_CACHE_2 */,
	"CACHE 1" /* LEVEL_CACHE_1 */, "CORE   " /* LEVEL_CORE    */};

/** Set to 1 once a first extls_init() call succeed */
static char initialized = 0;
/** default context created to make the lib transparent if no thread engine are used */
static extls_ctx_t ctx_root;

/** Used by HLS module to print the topology if the user requested it (export EXTLS_DUMP_TOPOLOGY) */
FILE* fd = NULL;
/** the lock on the topology file */
extls_lock_t fd_lock = EXTLS_LOCK_INITIALIZER;
/** Set to 1 if optimized TLS are supported */
short tlsopt_support = 1;

/**
 * Entry point for the library, initializing the components
 *
 * \warning This function has to be called anyway!
 * \return
 * <ul>
 * <li><b>EXTLS_EARCH</b> if the current architecture is not supported for optimized TLS access.</li>
 * <li><b>EXTLS_ENFIRST</b> it it is not the first call to extls_init()</li>
 * <li><b>EXTLS_SUCCESS</b> otherwise 
 * </ul>
 */
extls_ret_t extls_init(void)
{
	extls_init_verbosity();
	if(initialized) return EXTLS_ENFIRST;

	extls_info("Library: Initialization");

	if(getenv("EXTLS_ENABLE_VALGRIND") )
	{
		tlsopt_support = 0;
	}

	extls_ret_t ret;
	if(tlsopt_support && (ret = extls_optim_tls_check_method()))
	{
		extls_warn("Optimized TLS unsupported/disabled for this architecture !");
	}
	else
	{
		extls_info("Optim TLS: Architecture supported by the library");
	}

#if defined(HAVE_TOPOLOGY) && defined(ENABLE_HLS)
	extls_info("HLS: Enabling Topological TLS support");
	const char* file = getenv("EXTLS_DUMP_TOPOLOGY");
	if(file)
	{
		fd = fopen(file, "w");
		fprintf(fd, "digraph G{\n");
	}
	extls_hls_topology_init();
#endif

	extls_locate_dynamic_initializers();

	(void)extls_register_tls_segments();
	extls_ctx_init(&ctx_root, NULL);
	extls_ctx_restore(&ctx_root);

#if defined(HAVE_TOPOLOGY) && defined(ENABLE_HLS)
	extls_ctx_bind(&ctx_root, 0);
#endif
	initialized = 1;
	return EXTLS_SUCCESS;
}

/**
 * Entry point for the library, initializing the components
 * This is the function to handle accesses before main
 */
__attribute__((constructor)) void extls_constructor()
{
	extls_init();
}


/**
 * Close the library at the end of program.
 * 
 * It is not mandatory to call this function if it is the end of the program but
 * it is a good habit to do it. If not called, the topology file will not be
 * flushed and potential HLS structs won't be freed
 *
 * \return
 * <ul>
 * <li><b>EXTLS_SUCCESS</b> in any way<li>
 * </ul>
 */
extls_ret_t extls_fini(void)
{
	extls_info("Library: Finalization");
#if defined(HAVE_TOPOLOGY) && defined(ENABLE_HLS)
	extls_hls_topology_fini();
	if(fd)
	{
		fprintf(fd, "}\n");
		fclose(fd);
	}
#endif
	return EXTLS_SUCCESS;
}

/**
 * create a new context group.
 *
 * This function opens the group file to register any TLS registered with the
 * proper call: \see extls_ctx_grp_registering
 *
 * \param grp a pointer to the grp to create
 * \return
 * <ul>
 * <li><b>EXTLS_EINVAL</b> if grp is null </li>
 * <li><b>EXTLS_EOPEN</b> if the file cannot be created</li>
 * <li><b>EXTLS_SUCCESS</b> otherwise </li>
 * </ul>
 */
extls_ret_t extls_ctx_grp_open(extls_ctx_grp_t* grp)
{
	if(grp == NULL) return EXTLS_EINVAL;
	extls_info("Context Group: Initialization");

	grp->fd_name = malloc(sizeof(char) * 1024);
	snprintf(grp->fd_name, 1024, "/tmp/extls_ctx_grp_%lu_reg_segmt_%d", (unsigned long)grp, getpid());
	grp->fd = open(grp->fd_name, O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);

	if(grp->fd < 0) return EXTLS_EOPEN;

	grp->dflt_reg_seg_size = 0;

	return EXTLS_SUCCESS;
}

/**
 * clone an existing context group.
 *
 * This function clones an existing context group and make it ready to register any TLS registered with the
 * proper call: \see extls_ctx_grp_registering
 *
 * \warning This function is not implemented yet but present in the API and returns EXTLS_ENIMPL.
 *
 * \param grp a pointer to the grp to clone
 * \param copy a pointer to the clone
 * \return
 * <ul>
 * <li><b>EXTLS_EINVAL</b> if grp or copy is null </li>
 * <li><b>EXTLS_EOPEN</b> if the file cannot be created</li>
 * <li><b>EXTLS_SUCCESS</b> otherwise </li>
 * </ul>
 */
extls_ret_t extls_ctx_grp_clone(extls_ctx_grp_t* grp, extls_ctx_grp_t* copy)
{
	if(grp == NULL || copy == NULL) return EXTLS_EINVAL;

	extls_dbg("Context group: Cloning");

	extls_not_impl();
	return EXTLS_ENIMPL;
}

/**
 * Close an existing group.
 *
 * After that, a context can be added to a group, not before. Indeed, we need all TLS to be registered
 * before create the first context in the group
 * \param grp the group to close
 * \return
 * <ul>
 * <li><b>EXTLS_EINVAL</b> if grp is NULL</li>
 * </ul>
 */
extls_ret_t extls_ctx_grp_close(extls_ctx_grp_t*grp)
{
	if(grp == NULL) return EXTLS_EINVAL;
	extls_info("Context group: closed. Ready to use");
	remove(grp->fd_name); /* copy on write access: do not close the fd */
	return EXTLS_SUCCESS;
}

/**
 * Registering a new TLS objects to be part of a context.
 *
 * This function allows to register a TLS. Each context belonging to this group will herit
 * from this special TLS segment. This is for thread engine use
 * 
 * \param grp the grp the TLS is attached to
 * \param def_start a memory address where the TLS init image starts (can be NULL)
 * \param def_sz a positive integer meaning the TLS size
 * \param id the function returns here a unique id to refer to this TLS
 *
 * \return
 * <ul>
 * <li><b>EXTLS_EINVAL</b> if grp is NULL or the file has not be opened </li>
 * <li><b>EXTLS_EWRITE</b> if unable to write in the file </li>
 * <li><b>EXTLS_SUCCESS</b> otherwise</li>
 * </ul>
 */
extls_ret_t extls_ctx_grp_object_registering(extls_ctx_grp_t* grp, extls_object_t def_start, extls_size_t def_sz, extls_object_id_t* id)
{
	if(grp == NULL) return EXTLS_EINVAL;
	if(grp->fd < 0) return EXTLS_EINVAL;
	ssize_t add = 0;

	extls_info("Context group: registering %lu bytes", def_sz);
	/* return the current block offset */
	(*id) = grp->dflt_reg_seg_size;

	if(def_start == NULL) /* if the user do not provide an initializer */
	{
		if(extls_write_zeros(grp->fd, def_sz) != EXTLS_SUCCESS)
		{
			return EXTLS_EWRITE;
		}
	}
	else
	{
		add = write(grp->fd, def_start, def_sz);
		if(add < 0) return EXTLS_EWRITE;
	}

	grp->dflt_reg_seg_size += def_sz;

	return EXTLS_SUCCESS;
}

/**
 * Make a part of context initialization: registered TLS part.
 *
 * Copy from grp context, the registered TLS segment if it exists
 *
 * \param ctx a pointer to the context to init
 * \param grp the referent context group (can be NULL)
 *
 * \return
 * <ul>
 * <li><b>EXTLS_ENOMEM</b> if the map failed </li>
 * <li><b>EXTLS_SUCCESS</b> otherwise </li>
 * </ul>
 */
static inline extls_ret_t extls_ctx_init_reg_tls(extls_ctx_t* ctx, extls_ctx_grp_t* grp)
{
	ctx->reg_seg = NULL;
	ctx->reg_seg_size = 0;
	extls_dbg("Context: Registered block Initialization");
	if(grp != NULL)
	{
		ctx->reg_seg_size = grp->dflt_reg_seg_size;
		if(ctx->reg_seg_size > 0)
		{
			ctx->reg_seg = mmap(NULL,ctx->reg_seg_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, grp->fd, 0);
			if(ctx->reg_seg == MAP_FAILED) return EXTLS_ENOMEM;
		}
	}
	return EXTLS_SUCCESS;
}

/**
 * Make a part of context initialization: vector part.
 *
 * Create the TLS vector for the current context. Init from scratch any TLS levels.
 *
 * \param ctx the context to init
 * \param grp the parent group (which can be NULL)
 *
 * \return
 * <ul>
 * <li><b>EXTLS_SUCCESS</b> in any case </li>
 * </ul>
 */
static inline extls_ret_t extls_ctx_init_vector_tls(extls_ctx_t* ctx, extls_ctx_grp_t* grp)
{
	extls_dbg("Context: Vector Initialization");
	/* Need to remove the file first (after registerings being proceeded) */
	ctx->parent_grp = grp;

	/* create the table */
	ctx->tls_vector = malloc(sizeof(extls_object_level_t) * LEVEL_MAX);
	
	extls_object_level_type_t i;
	for(i=0; i < LEVEL_TLS_MAX; i++)
	{
		extls_object_level_t* level_obj = &ctx->tls_vector[i];
		/* create static segment */
		extls_map_static_segment(level_obj);

		/* create dynamic segment */
		level_obj->dyn_seg = calloc(EXTLS_DEFAULT_DYN_SEGMENTS, sizeof(extls_object_t*));
		level_obj->nb_dyn_seg = malloc(sizeof(extls_size_t));
		*(level_obj->nb_dyn_seg) = 0;
		level_obj->dyn_lock = malloc(sizeof(extls_lock_t));
		extls_lock_init(level_obj->dyn_lock, NULL);

		/* this level has no HLS data */
		level_obj->hls_data = NULL;
	}

#if defined(HAVE_TOPOLOGY) && defined(ENABLE_HLS)
	/* memset to 0, this is handled by ctx_bind() */
	memset(ctx->tls_vector+LEVEL_TLS_MAX,0, sizeof(extls_object_level_t) * (LEVEL_MAX - LEVEL_TLS_MAX));
#endif

	return EXTLS_SUCCESS;
}

/**
 * Init a context from scratch.
 *
 * This call is equivalent to herit from LEVELdd_PROCESS.
 *
 * \param ctx the context to init
 * \param grp the parent group (can be NULL)
 *
 * \return
 * <ul>
 * <li><b>EXTLS_EINVAL</b> if ctx is NULL</li>
 * <li><b>EXTLS_SUCCESS</b> if the init succeed </li>
 * <li><b>UNDEFINED</b> otherwise </li>
 * </ul>
 */
extls_ret_t extls_ctx_init(extls_ctx_t* ctx, extls_ctx_grp_t* grp)
{
	if(ctx == NULL) return EXTLS_EINVAL;
	extls_info("Context: Initialization");

	/* Only set if current context is bound to a pu (w/ extls_ctx_bind()) */
	ctx->pu = -1;
	ctx->async_gen_num = 0;
	ctx->sync_gen_num = 0;


	return
		extls_ctx_init_vector_tls(ctx, grp) == EXTLS_SUCCESS
		&& extls_ctx_init_reg_tls(ctx, grp) == EXTLS_SUCCESS
		;
}

/**
 * Create a clone of a context for a specific level.
 *
 * Create a copy, where each level higher than "level" will be herited", the others will
 * be recreated.
 *
 * \param ctx the ctx to clone
 * \param herit the clone
 * \param level the heritage level
 *
 * \return
 * <ul>
 * <li><b>EXTLS_EINVAL</b> if herit is NULL or level higher than LEVEL_TLS_MAX </li>
 * <li><b>EXTLS_ENOMEM</b> if unable to map the memory </li>
 * <li><b>EXTLS_SUCCESS</b> otherwise </li>
 * </ul>
 */
extls_ret_t extls_ctx_herit(extls_ctx_t* ctx, extls_ctx_t* herit, extls_object_level_type_t level)
{
	extls_ret_t ret = EXTLS_SUCCESS;

	if(herit == NULL || level >= LEVEL_TLS_MAX)
	{
		ret = EXTLS_EINVAL;
		goto ret_func;
	}

	if(ctx == NULL) /* herit from our root */
	{
		ctx = &ctx_root;
	}

	extls_info("Context: Creating Derived Object from level %s", LEVEL_NAME(level));

	herit->parent_grp = ctx->parent_grp;
	herit->async_gen_num = ctx->async_gen_num;
	herit->sync_gen_num = ctx->sync_gen_num;
	
	/* Clone registered TLS segment w/ initialization values ! */
	ret = extls_ctx_init_reg_tls(herit, ctx->parent_grp);

	if(ret != EXTLS_SUCCESS)
	{
		goto ret_func; //ret set by extls_ctx_ini_reg_tls()
	}

	herit->tls_vector = malloc(sizeof(extls_object_level_t) * LEVEL_MAX);

	if(!herit->tls_vector)
	{
		ret = EXTLS_ENOMEM;
		goto ret_func;
	}

	/* Clone user TLS segment w/ currently set ones by ctx (pointer segment copy) */
	extls_object_level_type_t i;
	for(i=0; i < LEVEL_TLS_MAX; i++)
	{
		/*extls_dbg("Level %s",LEVEL_NAME(i));*/
		if(level > i) /* we have to herit from this level */
		{
			/* herit the parent struct */
			herit->tls_vector[i] = ctx->tls_vector[i];

		}
		else /* we have to create a new chunk */
		{
			extls_object_level_t* level_obj = herit->tls_vector + i;
			extls_map_static_segment(level_obj);
			level_obj->dyn_seg = calloc(EXTLS_DEFAULT_DYN_SEGMENTS, sizeof(extls_object_t*));
			level_obj->nb_dyn_seg = malloc(sizeof(extls_size_t));
			*(level_obj->nb_dyn_seg) = 0;
			level_obj->dyn_lock = malloc(sizeof(extls_lock_t));
			extls_lock_init(level_obj->dyn_lock, NULL);
			level_obj->hls_data = NULL;
		}
	}
#if defined(HAVE_TOPOLOGY) && defined(ENABLE_HLS)
	/* We have to memset() first, to be sure that non-existent level will be void
	 * For example, if the topology does not have a NUMA node level, the struct is set w/ '0'
	 */
	memset(herit->tls_vector+LEVEL_TLS_MAX, 0,sizeof(extls_object_level_t) * (LEVEL_MAX - LEVEL_TLS_MAX));

	/* Note: binding are not herited as it would force libextls to pay the price of 
	 * a full migration when (if?) extls_ctx_bind() is called after that.
	 * This tricky implies an undefined behavior when accessing HLS between _herit() and _bind() calls.
	 * If a fatal() could be emitted when using TLS function calls, libextls cannot avoid a
	 * potential SEGV when relying on Optimized accesses (yet)
	 */
#endif

ret_func:
	return ret;
}

/**
 * attach a context to a hardware process unit.
 *
 * Used to fix a context to a topological branch -> enabling HLS for this context
 * \param ctx the ctx to bind
 * \param pu a positive value
 *
 * \return
 * <ul>
 * <li><b>EXTLS_EINVAL</b> without HLS support </li>
 * <li><b>EXTLS_SUCCESS</b> otherwise </li>
 * </ul>
 */
extls_ret_t extls_ctx_bind(extls_ctx_t* ctx, int pu)
{
#if defined(HAVE_TOPOLOGY) && defined(ENABLE_HLS)
	extls_info("Context: Binding");
	ctx->pu = pu;
	extls_hls_init_levels(ctx->tls_vector, pu);
	return EXTLS_SUCCESS;
#else
	UNUSED(ctx);
	UNUSED(pu);
	extls_warn("No need to bind a context when HLS support is disabled");
	return EXTLS_EINVAL;
#endif
}

/**
 * Free a context.
 *
 * \param ctx the context to free
 * \return
 * <ul>
 * <li><b>EXTLS_EINVAL</b> if ctx is NULL </li>
 * <li><b>EXTLS_ENOMEM</b> if unmap failed </li>
 * <li><b>EXTLS_SUCCESS</b> otherwise </li>
 * </ul>
 */
extls_ret_t extls_ctx_destroy(extls_ctx_t* ctx)
{
	if(ctx==NULL) return EXTLS_EINVAL;
	extls_info("Context: Destruction");
	if(ctx->reg_seg)
	{
		if(munmap(ctx->reg_seg, ctx->reg_seg_size) == -1)
		{
			return EXTLS_ENOMEM;
		}
	}

	return EXTLS_SUCCESS;
}

/**
 * Saving a context.
 *
 * This function does nothing. The symbol exists just to be symmetric with restore().
 * 
 * \param ctx the context to save
 * \return
 * <ul>
 * <li><b>EXTLS_EINVAL</b> if ctx is NULL</li>
 * <li><b>EXTLS_SUCCESS</b> otherwise</li>
 * </ul>
 */
extls_ret_t extls_ctx_save(extls_ctx_t* ctx)
{
	if(ctx == NULL) return EXTLS_EINVAL;
	extls_ret_t ret = EXTLS_SUCCESS;

	extls_dbg("Context: Saving");

	return ret;
}

/**
 * Restoring a context in the global address.
 *
 * Set the current context TLS vector in the global storage, context-switched by thread engine
 *
 * \param ctx the context to restore
 *
 * \return
 * <ul>
 * <li><b>EXTLS_EINVAL</b> if ctx is NULL</li>
 * <li><b>EXTLS_EARCH</b> if the architecture does not support optimized TLS (or not implemented)</li>
 * <li><b>EXTLS_SUCCESS</b> otherwise </li>
 * </ul>
 */
extls_ret_t extls_ctx_restore(extls_ctx_t* ctx)
{
	if(ctx == NULL) return EXTLS_EINVAL;
	extls_dbg("Context: Restoring");
	extls_ret_t ret;
	/* get the address where the current context is stored */
	extls_ctx_t** ctx_addr = (extls_ctx_t**) extls_get_context_storage_addr();
	/* store the address of context to restore */
	*ctx_addr = ctx;
	/* apply optim TLS storage */
	ret = extls_optim_tls_set_pointer(ctx->tls_vector);

	return ret;
}

/**
 * Get the TLS addr for a registered TLS.
 *
 * \param ctx the context where TLS value will be extracted
 * \param id the id provided by the library when the TLS has been registered
 * \param obj the TLS addr (out)
 *
 * \return
 * <ul>
 * <li><b>EXTLS_EINVAL</b> if ctx is NULL or id is not a valid id for this segment</li>
 * <li><b>EXTLS_SUCESS</b> otherwise </li>
 * </ul>
 */
extls_ret_t extls_ctx_reg_get_addr(extls_ctx_t* ctx, extls_object_id_t id, extls_object_t* obj)
{
	if(ctx == NULL) return EXTLS_EINVAL;
	if(id >= ctx->reg_seg_size) return EXTLS_EINVAL;

	extls_dbg("Context: Get registered block");
	(*obj) = ((void*)ctx->reg_seg + (unsigned long)id);
	return EXTLS_SUCCESS;
}

