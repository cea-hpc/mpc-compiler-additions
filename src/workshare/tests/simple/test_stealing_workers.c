#include <mpi.h>
#include <unistd.h>
#include <ws_test.h>
#include <omp.h>
void test_if_stealing(int node_rank, int num_threads)
{
  int i,sum = 0;
  if(node_rank == 0)
  {
#pragma ws for schedule(dynamic,1) reduction(+:sum)
    for(i=0;i<num_threads;i++)
    {
      sum++;
      sleep(1);
    }
    WS_ASSERT(sum,num_threads);
  }
}
int main(int argc, char ** argv)
{
  MPI_Init(&argc,&argv);
  int i,cw_rank, buf,size,sum=0;
  double t1,t2;
  int num_threads;
#pragma omp parallel
  {
    num_threads = omp_get_num_threads();
  }
  MPI_Comm node_comm;
  MPI_Comm_rank(MPI_COMM_WORLD,&cw_rank);
  MPI_Comm_size(MPI_COMM_WORLD,&size);
  MPI_Comm_split_type( MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, cw_rank,
                         MPI_INFO_NULL, &node_comm );

  int node_rank;
  MPI_Comm_rank( node_comm, &node_rank );
  MPI_Comm_free(&node_comm);
  int displs[size*size];
  int recvcounts[size*size];
  int recvbuf[size*size];

  for (i=0; i<size; i++) 
  {
    recvcounts[i] = 1;
    displs[i] = i*size;
  }

  MPI_Barrier(MPI_COMM_WORLD);
  t1=MPI_Wtime();
  test_if_stealing(node_rank,num_threads);
  MPI_Barrier(MPI_COMM_WORLD);
  t2=MPI_Wtime();
  WS_ASSERT_TIMER(t2-t1,1.5);

  MPI_Barrier(MPI_COMM_WORLD);
  t1=MPI_Wtime();
  test_if_stealing(node_rank,num_threads);
  MPI_Allreduce(&sum, &cw_rank, 1, MPI_INT, MPI_SUM,MPI_COMM_WORLD);
  t2=MPI_Wtime();
  WS_ASSERT_TIMER(t2-t1,1.5);

  MPI_Barrier(MPI_COMM_WORLD);
  t1=MPI_Wtime();
  test_if_stealing(node_rank,num_threads);
  MPI_Alltoall(&sum, 1,MPI_INT, recvbuf, 1, MPI_INT, MPI_COMM_WORLD);
  t2=MPI_Wtime();
  WS_ASSERT_TIMER(t2-t1,1.5);

  MPI_Barrier(MPI_COMM_WORLD);
  t1=MPI_Wtime();
  test_if_stealing(node_rank,num_threads);
  MPI_Alltoallv(&sum, recvcounts, displs,MPI_INT, recvbuf, recvcounts,displs,MPI_INT, MPI_COMM_WORLD);
  t2=MPI_Wtime();
  WS_ASSERT_TIMER(t2-t1,1.5);

  MPI_Barrier(MPI_COMM_WORLD);
  t1=MPI_Wtime();
  test_if_stealing(node_rank,num_threads);
  MPI_Allgather(&sum, 1,MPI_INT, recvbuf, 1, MPI_INT, MPI_COMM_WORLD);
  t2=MPI_Wtime();
  WS_ASSERT_TIMER(t2-t1,1.5);

  MPI_Barrier(MPI_COMM_WORLD);
  t1=MPI_Wtime();
  test_if_stealing(node_rank,num_threads);
  MPI_Allgatherv(&sum, 1,MPI_INT, recvbuf, recvcounts,displs,MPI_INT, MPI_COMM_WORLD);
  t2=MPI_Wtime();
  WS_ASSERT_TIMER(t2-t1,1.5);
//
  MPI_Barrier(MPI_COMM_WORLD);
  t1=MPI_Wtime();
  test_if_stealing(node_rank,num_threads);
  MPI_Scatter(&sum, 1,MPI_INT, &cw_rank, 1,MPI_INT, 0,MPI_COMM_WORLD);
  t2=MPI_Wtime();
  WS_ASSERT_TIMER(t2-t1,1.5);

  MPI_Barrier(MPI_COMM_WORLD);
  t1=MPI_Wtime();
  test_if_stealing(node_rank,num_threads);
  MPI_Scatterv(&sum, recvcounts, displs,MPI_INT, recvbuf, 1,MPI_INT, 0,MPI_COMM_WORLD);
  t2=MPI_Wtime();
  WS_ASSERT_TIMER(t2-t1,1.5);

  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Comm_rank(MPI_COMM_WORLD,&cw_rank);
  if(cw_rank == 0) WS_SUCCESS();
  MPI_Finalize();
  return 0;
}

