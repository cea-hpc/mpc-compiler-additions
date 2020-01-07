#define _GNU_SOURCE 1
#ifndef HAVE_EXTLS_SEGMT_HDLER_H
#define HAVE_EXTLS_SEGMT_HDLER_H

#include <elf.h>
#include <link.h>
#include "extls_common.h"
#include "extls_locks.h"

#if __WORDSIZE == 32
#define Elf_Phdr Elf32_Phdr
#else
#define Elf_Phdr Elf64_Phdr
#endif

/**
 * Description of information registered from a given TLS segment.
 *
 * When discovered, a TLS segment is register in a global table for later allocation.
 */
typedef struct extls_tls_seg_s
{
	const char * obj_name;   /**< shared object name */
	void * seg_addr;         /**< base address the segment is located */
	extls_size_t seg_filesz; /**< segment size in file => .tdata size */
	extls_size_t seg_memsz;  /**< segment size in mem  => .tdata + .tbsss sizes */
	extls_size_t seg_align;  /**< segment alignment */
	void* dyn_handler;       /**< handler set by the value returned by dlopen() if dynamic segment */
} extls_tls_seg_t;

/**
 * Struct used to transfer data through dl_iterate_phdr() call.
 *
 * It contains information about the segment to discover. If set, it contains the name of
 * the object which have to be opened. If found, the handler field is used
 */
typedef struct dyn_data_s {
	const char* name; /**< object name */
	void * handler;   /**< associated handler */
} dyn_data_t;

/* TLS segment array manipulation */
extls_size_t extls_get_nb_tls_segments(void);
extls_ret_t extls_register_tls_segments(void);
extls_ret_t extls_print_tls_segments(void);
extls_size_t extls_get_nb_static_tls_segments(void);
extls_size_t extls_get_sz_static_tls_segments(void);
extls_size_t extls_get_offset_for(extls_size_t module_id);

/* TLS copy segmen (for each caller) manipulation */
extls_ret_t extls_map_static_segment(extls_object_level_t* level);
extls_ret_t extls_map_dynamic_segment(extls_object_level_t* level, extls_size_t dyn_module);

/* loading functions wrapper */
void * extls_dlopen(const char * filename, int flag);
void * extls_ldlsym(void* handle, const char*symbol, extls_object_level_type_t level);
void * extls_dlsym(void* handle, const char*symbol);
int extls_dlclose(void* handle);

#endif
