#include <autopriv_test.h>
#include <extls.h>
#include <extls_dynamic.h>
#include <stdio.h>

int a = 1;
static int* b = &a;

int get_remote_b();

int main(int argc, char ** argv)
{
	AP_UNUSED(argc);
	AP_UNUSED(argv);

	extls_ctx_t ctx;
	extls_ctx_init(&ctx, NULL);
	extls_call_dynamic_initializers();
	extls_call_static_constructors();

	int rb = get_remote_b();
	int lb = (b)?*b:-1;

	AP_ASSERT(rb, 2, "%d");
	AP_ASSERT(lb, 1, "%d");

	return 0;
}
