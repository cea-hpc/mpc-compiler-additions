#include <stdio.h>
#include <assert.h>
#include <extls.h>
int a;
#pragma hls node(a)
int b;
#pragma hls numa(b) level(2)
int c;
#pragma hls numa(c) level(1)
int d;
#pragma hls numa(d)
int e;
#pragma hls socket(e)
int f;
#pragma hls cache(f) level(3)
int g;
#pragma hls cache(g) level(2)
int h;
#pragma hls cache(h) level(1)
int i;
#pragma hls cache(i)
int j;
#pragma hls core(j)

int main(int argc, char *argv[])
{
	int var = 0;
	for(var = 0; var < 8;var++)
	{
		#pragma hls single(a)
		{
		a++;
		}
	}
	fprintf(stdout, "&a=%p - a=%d\n", &a, a);
	assert(a == 8);

	fprintf(stdout, "&b=%p - b=%d\n", &b, b);
	fprintf(stdout, "&c=%p - c=%d\n", &c, c);
	fprintf(stdout, "&d=%p - d=%d\n", &d, d);
	fprintf(stdout, "&e=%p - e=%d\n", &e, e);
	#pragma hls barrier(f)
	fprintf(stdout, "&f=%p - f=%d\n", &f, f);
	fprintf(stdout, "&g=%p - g=%d\n", &g, g);
	fprintf(stdout, "&h=%p - h=%d\n", &h, h);
	fprintf(stdout, "&i=%p - i=%d\n", &i, i);

	#pragma hls single(j) nowait
	{
		++j;
	}
	fprintf(stdout, "&j=%p - j=%d\n", &j, j);
#pragma hls barrier(j)
	if(j!=1)
	{
		fprintf(stderr, "Error expected 1 and get %d\n", j);
		assert(NULL);
	}
	return 0;
}
