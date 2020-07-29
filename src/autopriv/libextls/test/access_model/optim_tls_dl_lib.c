#include <stdio.h>
__thread int i = 2;
__thread double j = 100.0;

void foo(void)
{
	i++;
	j++;
	printf("LIB ! &i=%p, &j=%p\n", &i, &j);
}
