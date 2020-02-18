#ifndef HAVE_EXTLS_COMMON_H
#define HAVE_EXTLS_COMMON_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <assert.h>
#include "extls_locks.h"
#include "extls_atomics.h"

/**
 * create an uniform, easy to maintain, definition for "size" type in the library
 */
typedef size_t extls_size_t;

/**
 * An object is a projection of a given address space. In most case, it is a
 * abstraction of void* type.
 */
typedef void* extls_object_t;

/**
 * an object id contains the value pointing to a registered TLS.
 */
typedef size_t extls_object_id_t;

/**
 * Enumation of different verbosity levels.
 */
typedef enum extls_verb_e
{
	EXTLS_VERB_NONE,  /**< Absolutely no prints, (even in case of crash) */
	EXTLS_VERB_ERROR, /**< Print only errors */
	EXTLS_VERB_WARN,  /**< prints all above + warnings (default) */
	EXTLS_VERB_INFO,  /**< prints all above + extls_info() */
	EXTLS_VERB_DEBUG  /**< prints everything */
} extls_verb_t;

/**
 * Enumration of different possible return values
 * Consider using extls_strerror* to parse the codes into readable strings.
 * \see extls_strerror extls_strerror_r
 */
typedef enum extls_return_type_e
{
	EXTLS_SUCCESS = 0, /**< Operation succeeded            */
	EXTLS_EINVAL,      /**< Invalid argument               */
	EXTLS_EAGAIN,      /**< Try again (busy)               */
	EXTLS_EFAULT,      /**< Will lead to SEGFLT            */
	EXTLS_ENOMEM,      /**< Not enough memory              */
	EXTLS_ENFOUND,     /**< Not found !                    */
	EXTLS_EARCH,       /**< architecture error             */
	EXTLS_ENFIRST,     /**< Operation has already been done*/
	EXTLS_EOPEN,       /**< Problems during file opening   */
	EXTLS_EWRITE,      /**< Problems during file writing   */
	EXTLS_ENIMPL,      /**< Not completely implemented yet */
	EXTLS_ENKNWN       /**< unstated error                 */
} extls_ret_t;

/**
 * Enumeration of different TLS/HLS levels handled by extls.
 */
typedef enum extls_object_level_type_e
{
	LEVEL_PROCESS,     /**< Depicts the object at process scope */
	LEVEL_TASK,        /**< Depicts the object at Task scope    */
	LEVEL_THREAD,      /**< Depicts the object at Thread scope  */
	LEVEL_OPENMP,      /**< Depicts the object at OpenMP scope  */

	LEVEL_TLS_MAX,     /**< used as enumerator boundary */
#if defined(HAVE_TOPOLOGY) && defined(ENABLE_HLS)
	/* to ensure synchronisation routines to work well,
	 * the HLS level order has to be the same as in gcc/coretypes.h
	 */
	LEVEL_NODE = LEVEL_TLS_MAX, /**< Depicts the object at node scope */
	LEVEL_NUMA_2,               /**< Depicts the object at NUMA scope */
	LEVEL_NUMA_1,               /**< Depicts the object at NUMA scope */
	LEVEL_SOCKET,               /**< Depicts the object at socket scope */
	LEVEL_CACHE_3,              /**< Depicts the object at cache scope */
	LEVEL_CACHE_2,              /**< Depicts the object at cache scope */
	LEVEL_CACHE_1,              /**< Depicts the object at cache scope */
	LEVEL_CORE,                 /**< Depicts the object at core scope */
	LEVEL_MAX

#else
	LEVEL_MAX = LEVEL_TLS_MAX
#endif
} extls_object_level_type_t;

/** defined in extls.c */
extern const char * const object_level_name[];

/** get the level value in a readable format */
#define LEVEL_NAME(u) object_level_name[u]
/** consider TLS blocks as "not initialized yet" */
#define UNALLOCATED_MODULE NULL
/** Max number of concurrently loaded dynamic segment supported by the library */
#define EXTLS_DEFAULT_DYN_SEGMENTS 8
/** round a to the next multiple of b */
#define roundup(a, b) (( ( a + (b-1) ) / b ) * b )

/**
 * Depicts the HLS data structure contained by an object level. \see struct extls_object_level_t
 * If the object level is a TLS level, the structure is not allocated (NULL).
 * Otherwise, it contains HLS informations, shown below:
 */
typedef struct extls_hls_data_s
{
	extls_atomic_int nb_toenter;    /**< number of ctx which have cloned the level */
	extls_atomic_int nb_entered;    /**< number of ctx reaching the current generation */
	volatile int sync_gen_num;      /**< number of previously reached barrier */
	extls_atomic_int async_gen_num; /**< number of previously readched async barrier */
} extls_hls_data_t;

/**
 * Depicts a TLS level. Contains static and dynamic structure for a given level
 */
typedef struct extls_object_level_s
{
	extls_object_t static_seg; /**< static projection of startup TLS blocks */
	extls_object_t** dyn_seg;  /**< array of TLS blocks dynamically loaded */
	extls_size_t* nb_dyn_seg;  /**< number of dynamic blocks (should be lower than EXTLS_DEFAULT_DYN_SEGMENTS) */
	extls_lock_t* dyn_lock;    /**< lock over dyn_seg array */
	extls_hls_data_t* hls_data;/**< set for HLS level, NULL otherwise */
} extls_object_level_t;

/**
 * Depicts a group of contexts, sharing the same initial registered segment.
 */
typedef struct extls_ctx_grp_s
{
	extls_size_t dflt_reg_seg_size; /**< registerd block size */
	char* fd_name;                  /**< file path */
	int fd;                         /**< file descriptor (copy-on-write)*/

} extls_ctx_grp_t;

/**
 * A context, containing information of the current computing unit. Especially,
 * it contains the tls vector (array of object levels)
 */
typedef struct extls_ctx_s
{
	extls_object_t reg_seg;           /**< registered TLS copy (from group) */
	extls_size_t reg_seg_size;        /**< its size */
	extls_object_level_t* tls_vector; /**< array of object level (user TLS)*/
	extls_ctx_grp_t* parent_grp;      /**< groups from which the context has been created */
	int pu;                           /**< Used by HLS model to register the bound process unit */
	int sync_gen_num;                 /**< Used by HLS model to count number of synchronous barrier encoutered */
	int async_gen_num;                /**< Used by HLS model to count number of asynchronous (nowait) encountered */
} extls_ctx_t;

/* set macro depending on colouring support */
#ifdef EXTLS_COLOR_ENABLED
#define BLU "\033[1;36m"
#define RED "\033[1;31m"
#define YEL "\033[1;33m"
#define GRE "\033[1;32m"
#define DEF "\033[0;0m"
#else
#define BLU ""
#define RED ""
#define YEL ""
#define GRE ""
#define DEF ""
#endif

/** macro to remove warnings when a function parameter is not used but have to be a parameter */
#ifndef UNUSED
#define UNUSED(a) (void)sizeof(a)
#endif
#define PRINT_TOPOLOGY(str,...) do{if(fd != NULL){extls_lock(&fd_lock); fprintf(fd, str, ##__VA_ARGS__);extls_unlock(&fd_lock);}}while(0)

#define extls_not_impl() do{extls_warn("Function %s() not implemented Yet !"DEF, __func__); }while(0)

#ifndef NDEBUG
/* different routines to print information at different verbosity levels */
#define extls_dbg(u,...)   do{if(extls_get_verbosity() >= EXTLS_VERB_DEBUG)fprintf(stderr, BLU "EXTLS-DEBUG: " u DEF" (%s():"RED"%d"DEF")\n", ##__VA_ARGS__, __FUNCTION__, __LINE__);}while(0)
#define extls_info(u,...)  do{if(extls_get_verbosity() >= EXTLS_VERB_INFO)fprintf(stderr, GRE "EXTLS-INFO : " u DEF"\n", ##__VA_ARGS__);}while(0)
#define extls_warn(u,...)  do{if(extls_get_verbosity() >= EXTLS_VERB_WARN)fprintf(stderr, YEL "EXTLS-WARN : " u DEF"\n", ##__VA_ARGS__);}while(0)
#define extls_fatal(u,...) do{if(extls_get_verbosity() >= EXTLS_VERB_ERROR)fprintf(stderr, RED "EXTLS-FATAL: " u DEF"\n", ##__VA_ARGS__);abort();}while(0)
#define extls_assert(u) do{if(!u) extls_fatal("Assertion " #u " returned false.");} while(0)

#else
#define extls_dbg(u,...)  (void)(u)
#define extls_info(u,...) (void)(u)
#define extls_warn(u,...)  do{fprintf(stderr, YEL "EXTLS-WARN : " u DEF"\n", ##__VA_ARGS__);}while(0)
#define extls_fatal(u,...) do{fprintf(stderr, RED "EXTLS-FATAL: " u DEF"\n", ##__VA_ARGS__);abort();}while(0)
#define extls_assert(u) (void)(u)
#endif

/* return code interpreter */
void extls_strerror_r(extls_ret_t type, char* str, size_t sz);
char *extls_strerror(extls_ret_t type);

/* misc */
extls_ret_t extls_write_zeros(int fd, extls_size_t sz);

/* verbosity */
void extls_init_verbosity(void);
extls_verb_t extls_get_verbosity(void);
void extls_set_verbosity(extls_verb_t set);

/* user-implemented function */
extern void*(*extls_get_context_storage_addr)(void);
extls_ret_t extls_set_context_storage_addr(void*(*)(void));
void* extls_get_dflt_context_storage_addr(void);
void extls_wait_for_value(volatile int*, int);

#if defined(HAVE_TOPOLOGY) && defined(ENABLE_HLS)
#include <extls_topo.h>
extern extls_topo_t* (*extls_get_topology_addr)(void);
extls_ret_t extls_set_topology_addr(extls_topo_t*(*)(void));
extls_topo_t* extls_get_dflt_topology_addr(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
