#ifndef HAVE_EXTLS_TYPES_H
#define HAVE_EXTLS_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

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
} extls_object_level_type_t;

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

#ifdef __cplusplus
}
#endif
#endif
