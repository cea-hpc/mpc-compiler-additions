#include <mpi.h>
#include <ws_test.h>
#define N 1000000
void add(int *a,int *b)
{
#pragma ws atomic
  (*a)++;
#pragma ws critical
  (*b)++;
}
int get_rank()
{
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  return rank;
}
int global_var;

int main(int argc, char ** argv)
{
  MPI_Init(&argc, &argv);
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  int local_var = 0,local_var2 = 0;
  int i,j,c=0, sum = 0;
  int tab[4];

  for(i=0;i<4;i++)tab[i]=0;
  int lastpriv,mul = 1;
  global_var =0;
  lastpriv=3;

#pragma ws for schedule(guided) private(j) reduction(+:global_var, tab[:4],c) firstprivate(mul) lastprivate(lastpriv) //shared(local_var)
  for(i=0;i<N;i++)
  {
    WS_ASSERT(mul,1);
    lastpriv = 5;
    c+=get_rank();
    global_var ++;
    if(i==-1) fprintf(stderr,"Bonjour !"); // Testing if global variable stderr not bugging

    add(&local_var,&local_var2);
    for(j=0;j<4;j++) 
      tab[j]++;

  }
  MPI_Barrier(MPI_COMM_WORLD);
  WS_ASSERT(lastpriv,5);
  WS_ASSERT(global_var,N);
  WS_ASSERT(local_var,N);
  WS_ASSERT(local_var2,N);
  for(j=0;j<4;j++) 
    WS_ASSERT(tab[j],N);

  WS_ASSERT(c,rank*N);

#pragma ws for reduction(*:mul)  
  for(i=0;i<20;i+=1)
  {
    mul*=2;
  }

  MPI_Barrier(MPI_COMM_WORLD);
  WS_ASSERT(mul,1048576);

  if(rank == 0) WS_SUCCESS();

  MPI_Finalize();

  return 0;

}

