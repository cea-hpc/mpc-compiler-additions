#include <config.h>
#include "extls_common.h"
#include "extls.h"
#include "extls_topo.h"

static extls_verb_t verbosity = EXTLS_VERB_WARN;/**< default verbosity level */
/** default context created to make the lib transparent if no thread engine are used */
static extls_ctx_t ctx_root;

/**
 * Set default verbosity level, depending on environment variable EXTLS_VERBOSE
 */
void extls_init_verbosity(void)
{
	char * verb = getenv("EXTLS_VERBOSE");
	if(!verb) return;
	else if(strcmp(verb, "none") == 0)
		verbosity = EXTLS_VERB_NONE;
	else if(strcmp(verb, "error") == 0)
		verbosity = EXTLS_VERB_ERROR;
	else if(strcmp(verb, "warn") == 0)
		verbosity = EXTLS_VERB_WARN;
	else if(strcmp(verb, "info") == 0)
		verbosity = EXTLS_VERB_INFO;
	else if(strcmp(verb, "debug") == 0)
		verbosity = EXTLS_VERB_DEBUG;
}

/**
 * retrieve the current verbosity level
 * \return the verbosity value
 */
extls_verb_t extls_get_verbosity(void)
{
	return verbosity;
}

/**
 * set the desired verbosity level
 * \param set the verobsity to set.
 */
void extls_set_verbosity(extls_verb_t set)
{
	assert(set >= EXTLS_VERB_NONE && set <= EXTLS_VERB_DEBUG);
	verbosity = set;
}

/** define strerror global string size */
#define strerror_sz 128
/** define local strerror buffer */
static char str[strerror_sz];

/**
 * parse a return code into a readable string
 *
 * By construction, this function is reentrant.
 *
 * \param type the return code to interpet
 * \param str the string to store the chain
 * \param sz the chain size
 */
void extls_strerror_r(extls_ret_t type, char* str, size_t sz)
{
	switch(type)
	{
		case EXTLS_SUCCESS:snprintf(str, sz, "Success\n"); break;
		case EXTLS_EINVAL :snprintf(str, sz, "Invalid Argument\n"); break;
		case EXTLS_EAGAIN :snprintf(str, sz, "Try Again\n"); break;
		case EXTLS_EFAULT :snprintf(str, sz, "Bad address - potential semgentation fault\n"); break;
		case EXTLS_ENOMEM :snprintf(str, sz, "Not enough memory!\n"); break;
		case EXTLS_ENFOUND:snprintf(str, sz, "Not found\n"); break;
		case EXTLS_EARCH  :snprintf(str, sz, "Error from Architecture support\n"); break;
		case EXTLS_ENFIRST:snprintf(str, sz, "Already done by an another call\n"); break;
		case EXTLS_EOPEN  :snprintf(str, sz, "Error in opening a file\n"); break;
		case EXTLS_EWRITE :snprintf(str, sz, "Error in writing in a file\n"); break;
		case EXTLS_ENKNWN :snprintf(str, sz, "Unknown error\n"); break;
		default           :snprintf(str, sz, "Not handled error in extls_sterror*() !\n"); break;
	}
}

/**
 * parse a return code into a readable string
 *
 * This function is not reentrant.
 *
 * \param type the type to interpet
 * \return a static string (no need to free)
 */
char *extls_strerror(extls_ret_t type)
{
	extls_strerror_r(type, str, strerror_sz);
	return str;
}

/**
 * Write a given number of zeros in a file
 *
 * \param fd an already-opened file
 * \param sz the number of zeros to write (as bytes)
 * \return
 * <ul>
 * <li><b>EXTLS_EWRITE</b> if the write() does not write "sz" bytes</li>
 * <li><b>EXTLS_SUCCESS</b> otherwise</li>
 * </ul>
 * \todo Maybe would it be better to simply use pwrite() in all the code.
 */
inline extls_ret_t extls_write_zeros(int fd, extls_size_t sz)
{
	extls_ret_t ret;

	char* zeros = calloc(sz, sizeof(char));
	ret = write(fd, zeros, sz * sizeof(char));
	free(zeros);

	return (ret == sz * sizeof(char)) ? EXTLS_SUCCESS : EXTLS_EWRITE;
}

void* extls_get_dflt_context_storage_addr(void)
{
	static __thread void* current_ctx = NULL;
	return &current_ctx;
}

void*(*extls_get_context_storage_addr)(void) = extls_get_dflt_context_storage_addr;
extls_ret_t extls_set_context_storage_addr(void*(*func)(void))
{
	extls_get_context_storage_addr = func;
	return EXTLS_SUCCESS;
}

/**
 * This function is used to replace the strong symbol if the thread engine does not define it.
 * \param addr_val the addr to watch
 * \param threshold the value to wait for
 */
#pragma weak extls_wait_for_value
void extls_wait_for_value(volatile int* addr_val, int threshold)
{
	while((*addr_val) != threshold)
	{
		sched_yield();
	}
}

extls_ctx_t* extls_fallback_ctx_get()
{
	return &ctx_root;
}

extls_ret_t extls_fallback_ctx_init()
{
	extls_ctx_init(&ctx_root, NULL);
	extls_ctx_restore(&ctx_root);
	return EXTLS_SUCCESS;
}

extls_ret_t extls_fallback_ctx_reset()
{
	extls_ctx_destroy(&ctx_root);
	extls_ctx_init(&ctx_root, NULL);
	return EXTLS_SUCCESS;

}
