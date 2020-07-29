#if !defined(_EXTLS_DL_H_)
#define _EXTLS_DL_H_

#include "extls.h"
#include "extls_types.h"

void * extls_dlopen(const char* filename, int flag);
void * extls_dlsym(void *handle, const char*symbol);
void * extls_ldlsym(void* handle, const char*symbol, extls_object_level_type_t level);
int extls_dlclose(void * handle);

#endif // _EXTLS_DL_H_
