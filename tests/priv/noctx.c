#include <autopriv_test.h>
#include <extls.h>

int i = 42;
int *j = &i;

int main(int argc, char const *argv[])
{
	AP_UNUSED(argc);
	AP_UNUSED(argv);

	AP_ASSERT(i, 42, "%d");
	AP_ASSERT(j, &i, "%p");
	AP_SUCCESS();
}
