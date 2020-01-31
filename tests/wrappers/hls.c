#include <stdio.h>
#include <assert.h>
#include <extls.h>
#include <extls_dynamic.h>

int i = 2;
#pragma hls node(i)

int main(int argc, char *argv[])
{
	extls_ctx_t ctx;

	extls_ctx_init(&ctx, NULL);
	extls_init();
	extls_call_dynamic_initializers();
	printf("i = %d, &i = %p\n", i, &i);
	assert(i == 2);
	extls_ctx_destroy(&ctx);
	
	return 0;
}
