#ifndef HAVE_EXTLS_ATOMICS_H
#define HAVE_EXTLS_ATOMICS_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef HAVE_ATOMICS
#include <opa_primitives.h>

typedef OPA_int_t extls_atomic_int;
#define extls_atomic_load           OPA_load_int
#define extls_atomic_store          OPA_store_int
#define extls_atomic_swap           OPA_swap_int
#define extls_atomic_incr           OPA_incr_int
#define extls_atomic_decr           OPA_incr_int
#define extls_atomic_fetch_and_incr OPA_fetch_and_incr_int
#define extls_atomic_fetch_and_decr OPA_fetch_and_decr_int
#define extls_atomic_cmp_and_swap   OPA_cas_int
#define extls_atomic_write_barrier  OPA_write_barrier
#else
typedef int extls_atomic_int;
#define extls_atomic_load(a) (*a)
#define extls_atomic_store(a, b) ((*a) = b)
#define extls_atomic_swap(a, b) do {extls_atomic_int __extls_temp = a; a = b; b = __extls_temp;} while(0) 
#define extls_atomic_incr(a) ((void)((*a)++))
#define extls_atomic_decr(a) ((void)((*a)--))
#define extls_atomic_fetch_and_incr(a) ((*a)++)
#define extls_atomic_fetch_and_decr(a) ((*a)--)
#define extls_atomic_cmp_and_swap(a, b, c) ((*a == b) ? (*a = c, 1) : (0) )
#define extls_atomic_write_barrier() 

#endif

#ifdef __cplusplus
}
#endif
#endif
