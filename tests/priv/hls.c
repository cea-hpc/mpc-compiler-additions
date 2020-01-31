#include <stdio.h>
#include <autopriv_test.h>
#include <extls.h>
#include <extls_dynamic.h>

int i = 2;
#pragma hls node(i)

int main(int argc, char *argv[])
{
	AP_UNUSED(argc);
	AP_UNUSED(argv);
	extls_ctx_t ctx;

	extls_ctx_init(&ctx, NULL);
	extls_init();
	extls_call_dynamic_initializers();
	printf("i = %d, &i = %p\n", i, &i);
	AP_ASSERT(i, 2, "%d");
	extls_ctx_destroy(&ctx);
	AP_SUCCESS();
	
	return 0;
}
