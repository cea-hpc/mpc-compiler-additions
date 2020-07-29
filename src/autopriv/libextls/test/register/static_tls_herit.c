#include <assert.h>
#include <extls.h>

struct my_obj
{
	int a;
	double b;
};

int main(int argc, char *argv[])
{
	struct my_obj def = {10, 20.2};
	
	extls_ctx_grp_t grp;
	extls_ctx_t ctx, herit;
	extls_object_id_t id1;
	extls_object_t obj1, obj2;

	assert(extls_init() == EXTLS_SUCCESS);

	assert(extls_ctx_grp_open(&grp) == EXTLS_SUCCESS);
	assert(extls_ctx_grp_object_registering(&grp,&def, sizeof(struct my_obj), &id1) == EXTLS_SUCCESS);
	assert(extls_ctx_grp_close(&grp) == EXTLS_SUCCESS);

	assert(extls_ctx_init(&ctx, &grp) == EXTLS_SUCCESS);
	assert(extls_ctx_herit(&ctx, &herit) == EXTLS_SUCCESS);

	assert(extls_ctx_static_get_addr(&ctx, id1, &obj1 ) == EXTLS_SUCCESS);

	struct my_obj tls1 = *((struct my_obj*)obj1);

	tls1.a = 100;
	tls1.b = 42.42;
	
	assert(extls_ctx_static_get_addr(&herit, id1, &obj2 ) == EXTLS_SUCCESS);
	struct my_obj tls2 = *((struct my_obj*)obj2);

	fprintf(stderr, "My TLS-1: {a=%d, b=%.2f} at %p\n", tls1.a, tls1.b, obj1);
	fprintf(stderr, "My TLS-2: {a=%d, b=%.2f} at %p\n", tls2.a, tls2.b, obj2);

	assert(extls_ctx_destroy(&ctx) == EXTLS_SUCCESS);
	assert(extls_fini() == EXTLS_SUCCESS);
	return 0;
}
