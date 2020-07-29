#include <assert.h>
#include <extls.h>

struct my_obj
{
	int a;
	double b;
};

int main(int argc, char *argv[])
{
	extls_ctx_grp_t grp;
	extls_ctx_t ctx;
	extls_object_t obj1;

	assert(extls_init() == EXTLS_SUCCESS);

	assert(extls_ctx_grp_open(&grp) == EXTLS_SUCCESS);
	assert(extls_ctx_grp_close(&grp) == EXTLS_SUCCESS);

	assert(extls_ctx_init(&ctx, &grp) == EXTLS_SUCCESS);

	assert(extls_ctx_static_get_addr(&ctx, 0, &obj1 ) == EXTLS_EINVAL);
	assert(extls_ctx_static_get_addr(&ctx, -1, &obj1 ) == EXTLS_EINVAL);
	assert(extls_ctx_static_get_addr(&ctx, 100, &obj1 ) == EXTLS_EINVAL);

	assert(extls_ctx_destroy(&ctx) == EXTLS_SUCCESS);
	assert(extls_fini() == EXTLS_SUCCESS);
	return 0;
}
