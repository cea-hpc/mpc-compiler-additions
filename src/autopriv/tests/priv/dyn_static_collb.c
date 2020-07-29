int zz = 2;
static int* b = &zz;


int get_remote_b()
{
	if(!b)
		return -1;
	return *b;
}
