#include <stdio.h>
#include <pthread.h>
#include <autopriv_test.h>
#include <extls.h>
#include <extls_dynamic.h>
#include <getopt.h>
#include <assert.h>
#include <string.h>

#define count 100

void* func(void* arg)
{
	extls_ctx_t child;

	extls_ctx_herit(NULL, &child, LEVEL_TASK);
	extls_ctx_restore(&child);

	extls_call_dynamic_initializers();


	int argc = 6;
	char * argv[] = {"./lol", "-a", "-b", "-c", "-d", "FIRSTARG"};


	/* Proceed with getopt parsing */
	char options[]="abcd";
	int c;

	int nb_found = 0;

	// tant qu'il reste des options
	while( (c=getopt (argc, argv, options)) != EOF )
	{
		printf("OPTIONS is %c\n", c);
		AP_ASSERT(c, options[nb_found], "%c");
		nb_found++;
	}

	AP_ASSERT(nb_found, 4, "%d");

	AP_ASSERT(strcmp(argv[optind], "FIRSTARG"), 0, "%d");

	extls_ctx_destroy(&child);
	return NULL;
}
int main(int argc, char *argv[])
{
	extls_ctx_t root;
	extls_ctx_init(&root, NULL);
	pthread_t t[count];

	unsigned long int i;
	for(i = 0; i < count; i++)
	{
		pthread_create(t + i, NULL, func, (void*)i);
	}

	for (i = 0; i < count; ++i) {
		pthread_join(t[i], NULL);
	}

	extls_ctx_destroy(&root);

	return 0;
}