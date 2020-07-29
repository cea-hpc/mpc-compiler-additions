#include <autopriv_test.h>
#include <extls.h>
#include <extls_dynamic.h>
int i = 42;

int main(int argc, char const *argv[])
{
	AP_UNUSED(argc);
	AP_UNUSED(argv);
	extls_ctx_t ctx;

	extls_ctx_init(&ctx, NULL);
	AP_ASSERT(i, 42, "%d");
	extls_ctx_destroy(&ctx);
	AP_SUCCESS();
}
