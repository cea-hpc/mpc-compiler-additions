#include <stdio.h>
#include <extls.h>
#include <assert.h>


#define print_vars() fprintf(stderr, "&process=%p(%d), &task=%p(%d), &thread=%p(%d), &omp=%p(%d)\n", &process,process, &task,task, &thread, thread, &omp, omp)

#define test_clone(a) \
{ \
	extls_ctx_t clone;\
	extls_ctx_herit(&root, &clone, a);\
	extls_ctx_restore(&clone);\
	process++;task++;thread++;omp++;\
	fprintf(stderr, #a ":\t ");\
	print_vars();\
	switch(a){\
		case LEVEL_PROCESS: \
			assert(root_process != &process);\
			assert(root_task    != &task);\
			assert(root_thread  != &thread);\
			assert(root_omp     != &omp);\
			break;\
		case LEVEL_TASK: \
			assert(root_process == &process);\
			assert(root_task    != &task);\
			assert(root_thread  != &thread);\
			assert(root_omp     != &omp);\
			break;\
		case LEVEL_THREAD: \
			assert(root_process == &process);\
			assert(root_task    == &task);\
			assert(root_thread  != &thread);\
			assert(root_omp     != &omp);\
			break;\
		case LEVEL_OPENMP: \
			assert(root_process == &process);\
			assert(root_task    == &task);\
			assert(root_thread  == &thread);\
			assert(root_omp     != &omp);\
			break;\
	}\
} 

extern __process int process;
extern __task int task;
extern __thread int thread;
extern __openmp int omp;

int main(int argc, char *argv[])
{
	extls_ctx_t root;

	/*extls_init();*/
	extls_ctx_init(&root, NULL);
	extls_ctx_restore(&root);
	
	int* root_process = &process;
	int* root_task = &task;
	int* root_thread = &thread;
	int* root_omp = &omp;
	
	process++;
	task++;
	thread++;
	omp++;
	
	fprintf(stderr, "ROOT:\t\t ");\
	print_vars();
	/*fprintf(stderr, "ROOT static addr: %p\n", root.tls_vector[LEVEL_PROCESS].static_seg);*/
	test_clone(LEVEL_PROCESS);
	test_clone(LEVEL_TASK);
	test_clone(LEVEL_THREAD);
	test_clone(LEVEL_OPENMP);

	/*extls_fini();*/

	fprintf(stderr, "Test Successful !\n");
	return 0;
}
