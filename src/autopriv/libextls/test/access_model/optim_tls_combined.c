#include <stdio.h>

int j;
extern int i;

int main(int argc, char *argv[])
{

	i++;
	j++;
	printf("i=%d(%p), j=%d(%p)\n", i, &i, j, &j);
	return 0;
}
