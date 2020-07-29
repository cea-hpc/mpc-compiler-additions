#include <extls_dynamic.h>
#include <iostream>
#include <unistd.h>
using namespace std;

int i = 2;
int* foo()
{
	return &i;
}

int* j = foo();

int main(int argc, char *argv[])
{
	extls_init();
	extls_call_dynamic_initializers();
	cout << "i = " << i << " &i = " << &i << endl;
	cout << "j = " << j << " &j = " << &j << endl;
	extls_fini();
	
	return 0;
}
