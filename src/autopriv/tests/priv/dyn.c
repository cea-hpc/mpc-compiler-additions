#include <autopriv_test.h>
#include <extls.h>
#include <extls_dynamic.h>
int i = 42;
int *j = &i;

int main(int argc, char const *argv[])
{
	AP_UNUSED(argc);
	AP_UNUSED(argv);

	extls_ctx_t ctx;
	extls_ctx_init(&ctx, NULL);
	extls_call_dynamic_initializers();
	extls_call_static_constructors();
	
	
	AP_ASSERT(i, 42, "%d");
	AP_ASSERT(j, &i, "%p");

	extls_ctx_destroy(&ctx);
	AP_SUCCESS();
}
