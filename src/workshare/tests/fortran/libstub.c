#ifndef WORKSHARE_TEST_H_
#define WORKSHARE_TEST_H_

#include <stdio.h>
#include <stdlib.h>
#define WS_ASSERT(a, b) do { if(a != b) { fprintf(stderr, "Error: returned %d, expected %d (%s:%d)\n", a, b, __FILE__, __LINE__); abort();} } while(0)
#define WS_ASSERT_NOT_NULL(a) do { if(a == NULL) { fprintf(stderr, "Error: "#a" is NULL(%s:%d)\n", __FILE__, __LINE__); abort();} } while(0)
#define WS_ASSERT_TIMER(a,b) do { if(a > b) { fprintf(stderr, "Error: no stealing, loop took %lf and should take less than %2lf (%s:%d) \n",a,b,__FILE__, __LINE__); abort();} } while(0)

#define WS_UNUSED(a) (void)(a)
#define WS_SUCCESS() do {printf("SUCCESS !\n"); }while(0)

extern struct
{
   int x;
} x_;
int x;
void MPC_Workshare_start(void (*func) (void*,long long,long long) , void* shareds,long long lb, long long ub, long long incr, int chunk_size, int steal_chunk_size, int scheduling_types, int nowait)
{
  x_.x++;
}
void MPC_Workshare_stop_stealing()
{
  x_.x++;
}

void MPC_Workshare_resteal()
{
  x_.x++;
}

void MPC_Workshare_atomic_start()
{
  x_.x++;
}

void MPC_Workshare_critical_start()
{
  x_.x++;
}
#endif