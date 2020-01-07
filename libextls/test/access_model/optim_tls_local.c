#include <mpi.h>
#include <stdio.h>

int i;

int main(int argc, char *argv[])
{
	fprintf(stderr, "i=%d, &i=%p\n", i, &i);
	i++;
	return 0;
}
