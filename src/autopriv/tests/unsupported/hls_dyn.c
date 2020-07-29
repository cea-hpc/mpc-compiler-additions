#include <stdio.h>
#include <autopriv_test.h>
#include <extls.h>
#include <extls_dynamic.h>

int i = 2;
int* j = &i;
#pragma hls node(j)

int main(int argc, char *argv[])
{
	AP_UNUSED(argc);
	AP_UNUSED(argv);
	extls_ctx_t ctx;

	extls_ctx_init(&ctx, NULL);
	extls_init();
	extls_call_dynamic_initializers();
	printf("i = %d, &i = %p\n", i, &i);
	printf("j = %p, &j = %p\n", j, &j);
	AP_ASSERT(i, 2, "%d");
	AP_ASSERT(j, &i, "%p");
	extls_ctx_destroy(&ctx);
	AP_SUCCESS();
	
	return 0;
}
