#define _GNU_SOURCE 1
#include <stdio.h>
#include <dlfcn.h>
#include <extls_segmt_hdler.h>
#include <pthread.h>

#define dlsym extls_dlsym
#define dlopen extls_dlopen
#define dlclose extls_dlclose

__process void * handle = NULL;

void * main_thread1(void * arg)
{
	int * i_ptr = (int*)extls_ldlsym(handle, "i", LEVEL_THREAD);
	double* j_ptr = (double*)extls_ldlsym(handle, "j", LEVEL_THREAD);
	/*int * i_ptr = (int*)dlsym(handle, "i");*/
	/*double* j_ptr = (double*)dlsym(handle, "j");*/
	(*i_ptr)++;
	(*j_ptr)++;

	printf("TH1: &i=%p, &j=%p\n", i_ptr, j_ptr);
}


void * main_thread2(void * arg)
{
	int * i_ptr = (int*)extls_ldlsym(handle, "i", LEVEL_THREAD);
	double* j_ptr = (double*)extls_ldlsym(handle, "j", LEVEL_THREAD);
	/*int * i_ptr = (int*)dlsym(handle, "i");*/
	/*double* j_ptr = (double*)dlsym(handle, "j");*/
	(*i_ptr)++;
	(*j_ptr)++;

	printf("TH2: &i=%p, &j=%p\n", i_ptr, j_ptr);
}

int main(int argc, char *argv[])
{
	extls_init();
	 handle = dlopen("./liba.so", RTLD_LAZY);
	


	 pthread_t th1, th2;

	 pthread_create(&th1, NULL, main_thread1, NULL);
	 pthread_create(&th2, NULL, main_thread2, NULL);


	 pthread_join(th1, NULL);
	 pthread_join(th2, NULL);
	
	dlclose(handle);
	extls_fini();
	return 0;
}
