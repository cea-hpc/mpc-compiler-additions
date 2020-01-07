#include <extls.h>
#include <stdio.h>

extern __process int process;
extern __task int task;
extern __thread int thread;
extern __openmp int omp;


#define exchange(u, l) \
	do{ \
		extls_ctx_restore(&u);\
		fprintf(stderr, "HERIT-%s: process=%p, task=%p, thread=%p, omp=%p\n", l, &process, &task, &thread, &omp);\
	} while(0)
int main(int argc, char *argv[])
{
	extls_init();

	extls_ctx_t root, herit_process, herit_task, herit_thread, herit_omp;

	extls_ctx_init(&root, NULL);
	extls_ctx_herit(&root, &herit_thread, LEVEL_THREAD);
	extls_ctx_herit(&root, &herit_task, LEVEL_TASK);
	extls_ctx_herit(&root, &herit_process, LEVEL_PROCESS);
	extls_ctx_herit(&root, &herit_omp, LEVEL_OPENMP);
	

	extls_ctx_restore(&root);
	fprintf(stderr, "ROOT         : process=%p, task=%p, thread=%p, omp=%p\n", &process, &task, &thread, &omp);

	exchange(herit_process, LEVEL_NAME(LEVEL_PROCESS));
	foo(LEVEL_NAME(LEVEL_PROCESS));
	exchange(herit_task, LEVEL_NAME(LEVEL_TASK));
	foo(LEVEL_NAME(LEVEL_TASK));
	exchange(herit_thread, LEVEL_NAME(LEVEL_THREAD));
	foo(LEVEL_NAME(LEVEL_THREAD));
	exchange(herit_omp, LEVEL_NAME(LEVEL_OPENMP));
	foo(LEVEL_NAME(LEVEL_OPENMP));


	extls_fini();
	return 0;
}
