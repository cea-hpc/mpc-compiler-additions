#ifndef HAVE_EXTLS_H
#define HAVE_EXTLS_H

#include "extls_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* library's main entry points */
extls_ret_t extls_init(void);
extls_ret_t extls_fini(void);

/* context group management */
extls_ret_t extls_ctx_grp_open(extls_ctx_grp_t* grp);
extls_ret_t extls_ctx_grp_close(extls_ctx_grp_t* grp);
extls_ret_t extls_ctx_grp_object_registering(extls_ctx_grp_t* grp, extls_object_t def_start, extls_size_t def_sz, extls_object_id_t* id);

/* context management */
extls_ret_t extls_ctx_init(extls_ctx_t* ctx, extls_ctx_grp_t* grp);
extls_ret_t extls_ctx_herit(extls_ctx_t* ctx, extls_ctx_t* herit, extls_object_level_type_t level);
extls_ret_t extls_ctx_bind(extls_ctx_t* ctx, int pu);
extls_ret_t extls_ctx_destroy(extls_ctx_t* ctx);
extls_ret_t extls_ctx_save(extls_ctx_t* ctx);
extls_ret_t extls_ctx_restore(extls_ctx_t* ctx);

extls_ret_t extls_ctx_reg_get_addr(extls_ctx_t* ctx, extls_object_id_t id, extls_object_t* obj);

/* Static TLS segment query */
extls_size_t extls_get_sz_static_tls_segments(void);

extls_ret_t extls_set_context_storage_addr(void*(*)(void));
extls_ret_t extls_set_topology_addr(void*(*)(void));

#ifdef __cplusplus
}
#endif
#endif
