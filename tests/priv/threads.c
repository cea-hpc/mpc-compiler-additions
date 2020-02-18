#include <pthread.h>
extern void*(*lol)(void);
#include <extls.h>
#include <extls_dynamic.h>
int i = 2;
int*j = &i;


#define count 100
__process void* array[count];

void* func(void* arg)
{
	int id = (unsigned long int)arg;
	extls_ctx_t child;

	extls_ctx_herit(NULL, &child, LEVEL_TASK);
	extls_ctx_restore(&child);

	extls_call_dynamic_initializers();

	array[id] = (void*)&i;
	//printf("&i = %p\n", &i);

	extls_ctx_destroy(&child);
	return NULL;
}
int main(int argc, char *argv[])
{
	extls_ctx_t root;
	extls_ctx_init(&root, NULL);
	pthread_t t[count];

	unsigned long int i, j, invalid = 0;
	for(i = 0; i < count; i++)
	{
		pthread_create(t + i, NULL, func, (void*)i);
	}

	for (i = 0; i < count; ++i) {
		pthread_join(t[i], NULL);
	}

	for (i = 0; i < count; ++i) {
		for (j = 0; j < count; ++j) {
			if(i != j)
				if(array[i] == array[j])
				{
					fprintf(stderr, "Same value for Th %lu & %lu (%p)\n", i, j, array[i]);
					invalid++;
				}
		}
	}

	extls_ctx_destroy(&root);

	fprintf(stderr, "Found %lu mismatch(es)\n", invalid );
	
	return invalid;
}
