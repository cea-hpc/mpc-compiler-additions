#include <mpi.h>
#include <ws_test.h>
#define N 1000000

int main(int argc, char ** argv)
{
  MPI_Init(&argc,&argv);
  int i,rank,sum=0;
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
#pragma ws for schedule(guided) reduction(+:sum)
  for(i=0;i<N;i++)
  {
    sum++;
  }
  MPI_Barrier(MPI_COMM_WORLD);
  WS_ASSERT(sum,N);

  sum = 0;
#pragma ws for schedule(dynamic,10) reduction(+:sum)
  for(i=0;i<N;i++)
  {
    sum++;
  }
  MPI_Barrier(MPI_COMM_WORLD);
  WS_ASSERT(sum,N);
  if(rank == 0) WS_SUCCESS();
  MPI_Finalize();
  return 0;
}
