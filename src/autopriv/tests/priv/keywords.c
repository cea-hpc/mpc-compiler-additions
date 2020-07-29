#include <autopriv_test.h>
#include <extls.h>

__process int i = 1;
__task int j = 2;
__thread int k = 3;
__openmp int l = 4;

int main(int argc, char const *argv[])
{
	AP_UNUSED(argc);
	AP_UNUSED(argv);

	extls_ctx_t ctx;
	extls_ctx_init(&ctx, NULL);
	
	AP_ASSERT(i, 1, "%d");
	AP_ASSERT(j, 2, "%d");
	AP_ASSERT(k, 3, "%d");
	AP_ASSERT(l, 4, "%d");

	extls_ctx_destroy(&ctx);
	AP_SUCCESS();
}
