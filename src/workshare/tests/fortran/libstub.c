#include <stdio.h>
#include <stdlib.h>

extern struct
{
   int x;
} x_;

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

void MPC_Workshare_critical_start()
{
  x_.x++;
}

void MPC_Workshare_critical_end()
{

}
