#include <assert.h>
#include <extls_segmt_hdler.h>
#include <extls.h>

__thread int i = 440394;
extern __thread int val;
#define create_alias(u) _Pragma(weak __lol__ ## u ## __lol__ = __lal__ ## u ## __lal__)
int main(int argc, char *argv[])
{

	assert(extls_init() == EXTLS_SUCCESS);
	
	assert(extls_register_tls_segments() == EXTLS_ENFIRST);
	assert(extls_print_tls_segments() == EXTLS_SUCCESS);

	void * proj = NULL;
	void * proj2 = NULL;

	assert(extls_map_tls_segment(0, &proj) == EXTLS_SUCCESS);
	assert(extls_map_tls_segment(1, &proj2) == EXTLS_SUCCESS);

	*((int*)proj) = 101;
	*((int*)proj2) = 100;
	printf("in projection, i=%d\n", *((int*)proj));
	printf("in projection, val=%d\n", *((int*)proj2));
	/*printf("in projection, val2=%2f\n", *((int*)proj2));*/
	
	printf("in main(), i=%d\n", i);
	printf("in main(), val=%d\n", val);
	
	assert(extls_fini() == EXTLS_SUCCESS);
	return 0;
}
