#include <assert.h>
#include <extls.h>
#include <extls_dynamic.h>
#include <stdio.h>

int i = 42;
int *j = &i;

int main(int argc, char const *argv[])
{
	extls_ctx_t ctx;
	extls_ctx_init(&ctx, NULL);
	extls_call_dynamic_initializers();
	extls_call_static_constructors();
	
	printf("i = %d, &i=%p\n", i, &i);
	printf("j = %p, &j=%p\n", j, &j);
	assert(i == 42);
	assert(j == &i);

	extls_ctx_destroy(&ctx);
	return 0;
}
