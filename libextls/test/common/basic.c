#include <assert.h>
#include <extls_segmt_hdler.h>
#include <extls.h>

__thread int i;
int main(int argc, char *argv[])
{
	assert(extls_init() == EXTLS_SUCCESS);
	
	assert(extls_register_tls_segments() == EXTLS_SUCCESS);
	assert(extls_print_tls_segments() == EXTLS_SUCCESS);
	
	assert(extls_fini() == EXTLS_SUCCESS);
	return 0;
}
