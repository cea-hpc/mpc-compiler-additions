#include <autopriv_test.h>
#include <extls.h>
#include <extls_dynamic.h>
#include <stdio.h>

int a = 1;
int b = 2;


struct toto
{
	int *a;
	int *b;
};

static struct toto lol = { & a , & b};

int main(int argc, char ** argv)
{
	AP_UNUSED(argc);
	AP_UNUSED(argv);

	extls_ctx_t ctx;
	extls_ctx_init(&ctx, NULL);
	extls_call_dynamic_initializers();
	extls_call_static_constructors();
	
	AP_ASSERT_NOT_NULL(lol.a);
	AP_ASSERT_NOT_NULL(lol.b);

	printf("a = %p\nb = %p\n", lol.a, lol.b);
	if( lol.a && lol.b)
	{
		AP_ASSERT(*lol.a, 1, "%d");
		AP_ASSERT(*lol.b, 2, "%d");
	}

	return 0;
}
