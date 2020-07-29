#include <extls.h>
#include <assert.h>

#define static_access(u, l) u.tls_vector[l].static_seg
#define dynamic_access(u,l) u.tls_vector[l].dyn_seg

int main(int argc, char *argv[])
{
	extls_init();

	extls_ctx_t init;
	extls_ctx_t herit_process, herit_task, herit_thread, herit_omp;

	extls_ctx_init(&init, NULL);
	extls_ctx_herit(&init, &herit_process, LEVEL_PROCESS);
	extls_ctx_herit(&init, &herit_task, LEVEL_TASK);
	extls_ctx_herit(&init, &herit_thread, LEVEL_THREAD);
	extls_ctx_herit(&init, &herit_omp, LEVEL_OPENMP);

	fprintf(stderr,"Static: check PROCESS level...\n");
	assert(static_access(init, LEVEL_PROCESS) == static_access(herit_omp, LEVEL_PROCESS));
	assert(static_access(init, LEVEL_PROCESS) == static_access(herit_thread, LEVEL_PROCESS));
	assert(static_access(init, LEVEL_PROCESS) == static_access(herit_task, LEVEL_PROCESS));
	assert(static_access(init, LEVEL_PROCESS) != static_access(herit_process, LEVEL_PROCESS));
	
	fprintf(stderr,"Static: check TASK level...\n");
	assert(static_access(init, LEVEL_TASK) == static_access(herit_omp, LEVEL_TASK));
	assert(static_access(init, LEVEL_TASK) == static_access(herit_thread, LEVEL_TASK));
	assert(static_access(init, LEVEL_TASK) != static_access(herit_task, LEVEL_TASK));
	assert(static_access(init, LEVEL_TASK) != static_access(herit_process, LEVEL_TASK));
	
	fprintf(stderr,"Static: check THREAD level...\n");
	assert(static_access(init, LEVEL_THREAD) == static_access(herit_omp, LEVEL_THREAD));
	assert(static_access(init, LEVEL_THREAD) != static_access(herit_thread, LEVEL_THREAD));
	assert(static_access(init, LEVEL_THREAD) != static_access(herit_task, LEVEL_THREAD));
	assert(static_access(init, LEVEL_THREAD) != static_access(herit_process, LEVEL_THREAD));
	
	fprintf(stderr,"Static: check OPENMP level...\n");
	assert(static_access(init, LEVEL_OPENMP) != static_access(herit_omp, LEVEL_OPENMP));
	assert(static_access(init, LEVEL_OPENMP) != static_access(herit_thread, LEVEL_OPENMP));
	assert(static_access(init, LEVEL_OPENMP) != static_access(herit_task, LEVEL_OPENMP));
	assert(static_access(init, LEVEL_OPENMP) != static_access(herit_process, LEVEL_OPENMP));

	fprintf(stderr,"Dynamic: check PROCESS level...\n");
	assert(dynamic_access(init, LEVEL_PROCESS) == dynamic_access(herit_omp, LEVEL_PROCESS));
	assert(dynamic_access(init, LEVEL_PROCESS) == dynamic_access(herit_thread, LEVEL_PROCESS));
	assert(dynamic_access(init, LEVEL_PROCESS) == dynamic_access(herit_task, LEVEL_PROCESS));
	assert(dynamic_access(init, LEVEL_PROCESS) != dynamic_access(herit_process, LEVEL_PROCESS));
	
	fprintf(stderr,"Dynamic: check TASK level...\n");
	assert(dynamic_access(init, LEVEL_TASK) == dynamic_access(herit_omp, LEVEL_TASK));
	assert(dynamic_access(init, LEVEL_TASK) == dynamic_access(herit_thread, LEVEL_TASK));
	assert(dynamic_access(init, LEVEL_TASK) != dynamic_access(herit_task, LEVEL_TASK));
	assert(dynamic_access(init, LEVEL_TASK) != dynamic_access(herit_process, LEVEL_TASK));
	
	fprintf(stderr,"Dynamic: check THREAD level...\n");
	assert(dynamic_access(init, LEVEL_THREAD) == dynamic_access(herit_omp, LEVEL_THREAD));
	assert(dynamic_access(init, LEVEL_THREAD) != dynamic_access(herit_thread, LEVEL_THREAD));
	assert(dynamic_access(init, LEVEL_THREAD) != dynamic_access(herit_task, LEVEL_THREAD));
	assert(dynamic_access(init, LEVEL_THREAD) != dynamic_access(herit_process, LEVEL_THREAD));
	
	fprintf(stderr,"Dynamic: check OPENMP level...\n");
	assert(dynamic_access(init, LEVEL_OPENMP) != dynamic_access(herit_omp, LEVEL_OPENMP));
	assert(dynamic_access(init, LEVEL_OPENMP) != dynamic_access(herit_thread, LEVEL_OPENMP));
	assert(dynamic_access(init, LEVEL_OPENMP) != dynamic_access(herit_task, LEVEL_OPENMP));
	assert(dynamic_access(init, LEVEL_OPENMP) != dynamic_access(herit_process, LEVEL_OPENMP));
	extls_fini();
	return 0;
}
