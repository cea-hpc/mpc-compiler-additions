#include <stdio.h>
#include <pthread.h>
#include <autopriv_test.h>
#include <extls.h>
#include <extls_dynamic.h>
#include <getopt.h>
#include <assert.h>
#include <string.h>
#include <extls.h>

#define count 100

int nb_found = 0;

void* func(void* arg)
{
	extls_ctx_t child;

	extls_ctx_herit(NULL, &child, LEVEL_TASK);
	extls_ctx_restore(&child);

	extls_call_dynamic_initializers();

	AP_ASSERT(optind, 5, "%d");

	extls_ctx_destroy(&child);
	return NULL;
}
int main(int argc, char *argv[])
{
	extls_ctx_t root;
	extls_ctx_init(&root, NULL);
	pthread_t t[count];

	int myargc = 6;
	char * myargv[] = {"./lol", "-a", "-b", "-c", "-d", "FIRSTARG"};


	/* Proceed with getopt parsing */
	char options[]="abcd";
	int c;


	// tant qu'il reste des options
	while( (c=getopt (myargc, myargv, options)) != EOF )
	{
		AP_ASSERT(c, options[nb_found], "%c");
		nb_found++;
	}

	AP_ASSERT(nb_found, 4, "%d");

	AP_ASSERT(strcmp(myargv[optind], "FIRSTARG"), 0, "%d");

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