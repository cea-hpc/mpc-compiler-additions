#define _GNU_SOURCE 1
#include <stdio.h>
#include <dlfcn.h>
#include <pthread.h>
struct st
{
	void ** handle;
	pthread_mutex_t* lock;
};

void * handler = NULL;
void * foo (void * arg)
{
	struct st* s= (struct st*)arg;
	void * sym = dlsym(*(s->handle), "i");
	int* i_thread = ((int*)sym);
	
	pthread_mutex_lock(s->lock);
	++(*i_thread);
	printf("i=%d, &i=%p\n", *i_thread, i_thread);
	pthread_mutex_unlock(s->lock);
}

int main(int argc, char *argv[])
{
	void * val = dlopen("liba.so", RTLD_LAZY);
	handler = val; 
	if(!handler)
	{
		printf("error dlopen()\n");
		return 1;
	}
	const short MAX = 5;
	pthread_t th[MAX];
	struct st s;
	s.handle = &handler;
	s.lock = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(s.lock, NULL);
	
	int j;
	for(j=0; j<MAX; j++)
	{
		pthread_create(&(th[j]), NULL, foo, &s);
	}

	for(j=0; j<MAX; j++)
		pthread_join(th[j], NULL);

	return 0;
}
