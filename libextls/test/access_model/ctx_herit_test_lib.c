#include <stdio.h>

__process int process = 100;
__task int task = 200;
__thread int thread = 300;
__openmp int omp=400;


void foo(char* name)
{
	fprintf(stderr, "LIB-%s  : process=%p, task=%p, thread=%p, omp=%p\n\n", name, &process, &task, &thread, &omp);
}
