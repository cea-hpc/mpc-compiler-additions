#include <ws_test.h>

int main(int argc, char ** argv)
{
  int i,j=0,sum=0;
#pragma ws for schedule(guided) reduction(+:sum)
  for(i=0;i<10;i++)
  {
    sum++;
  }

  WS_ASSERT(x,1);

#pragma ws for schedule(dynamic,10) private(j) 
  for(i=0;i<10;i++)
  {
    j++;
    sum++;
  }
  WS_ASSERT(x,2);

#pragma ws stopsteal
  WS_ASSERT(x,3);
#pragma ws resteal
  WS_ASSERT(x,4);
#pragma ws critical
  {
    i++;
  }
  
  WS_ASSERT(x,5);
  WS_SUCCESS();
  return 0;
}
