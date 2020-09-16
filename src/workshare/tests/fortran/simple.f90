program main

  use omp_lib
  use mpi


  implicit none

  real ( kind = 8 ) factor
  integer ( kind = 8 ) i,n
  integer ( kind = 4 ) error,num_procs,rank
  real ( kind = 8 ) wtime
  integer ( kind = 8 ), allocatable, dimension ( : ) :: x 
  integer(kind = 8)  xdoty 
  integer ( kind = 8 ), allocatable, dimension ( : ) :: y
  character(len=32) :: arg
  integer (kind = 8) testpriv

  call MPI_Init ( error )

  call MPI_Comm_size ( MPI_COMM_WORLD, num_procs, error )

  call MPI_Comm_rank ( MPI_COMM_WORLD, rank, error )
  call getarg(1,arg)
  n = 1000
  call sleep(mod(rank,3)) 
  !Making sure we have stealing by starting MPI ranks not at the same time

  allocate ( x(1:n) )
  allocate ( y(1:n) )

  factor = real ( n, kind = 8 )
  factor = 1.0D+00 / sqrt ( 2.0D+00 * factor * factor + 3 * factor + 1.0D+00 )

  do i = 1, n
  x(i) = i
  end do

  do i = 1, n
  y(i) = i
  end do
  xdoty = 0

  testpriv=7
  call test02 ( n, x, y, xdoty,testpriv )

  if(xdoty .ne. n*(n+1)/2) then
    print *, ''//achar(27)//'[31;1mFAILED !!! xdoty =', n*(n+1)/2,'expected', xdoty,''//achar(27)//'[0m'
    call MPI_Abort(MPI_COMM_WORLD,1, error)
  end if

  deallocate ( x )
  deallocate ( y )

  call MPI_Barrier(MPI_COMM_WORLD,error)
  if(rank .eq.0) then
    print *, ''//achar(27)//'[32;1mPASSED !'//achar(27)//'[0m'
  end if
  call MPI_Finalize(error)
  stop
  end
  subroutine test02 ( n, x, y, xdoty,testpriv )
    use mpi
    implicit none

    integer (kind=8) n

    integer ( kind = 4 ) i,j,error,myid,addr
    integer ( kind = 8 ) xdoty,sum1,sum2,sum3,sum4
    integer ( kind = 8 ) x(n)
    integer ( kind = 8 ) y(n)
    integer ( kind = 8 ) z(10)
    integer ( kind = 8 ) testpriv
    xdoty = 0.0D+00
    sum1 = 0
    sum2 = 0
    sum3 = 3
    sum4 = 0
    z=0
    y=0
    testpriv = 7
    addr = loc(testpriv)
    !$ws stopsteal
    !$ws resteal
    !$ws do reduction(+:y,z,xdoty,sum1,sum4) private(j) steal_schedule(guided,5) lastprivate(testpriv)
    do i = 1, n
    xdoty = xdoty + x(i) 
    sum1 = sum1 + 1 
    sum4 = sum4 + 1 

    if(loc(testpriv) .eq. addr) then
      print *, ''//achar(27)//'[31;1mFAILED !!! testpriv is not private' //achar(27)//'[0m'
      call MPI_Abort(MPI_COMM_WORLD,1, error)
    end if

    testpriv = 15
    !$ws atomic
    !$omp atomic
    sum2=sum2+1!x(i)

    !$ws critical
    !$omp critical
    sum3 = sum3 + 1
    x(i) = x(i)
    !$ws end critical
    !$omp end critical
    do j = 1, 10
    z(j) = z(j) + j
    y(j) = y(j) + j
    end do

    end do

    if(testpriv .ne. 15) then
      print *, ''//achar(27)//'[31;1mFAILED !!! testpriv =', testpriv, 'expected 15' //achar(27)//'[0m'
      call MPI_Abort(MPI_COMM_WORLD,1, error)
    end if

    if(sum1 .ne. n) then
      print *, ''//achar(27)//'[31;1mFAILED !!! sum1 =', sum1, 'n =',n,''//achar(27)//'[0m'
      call MPI_Abort(MPI_COMM_WORLD,1, error)
    end if

    if(sum2 .ne. n) then
      print *, ''//achar(27)//'[31;1mFAILED !!! sum2 =', sum2, 'n =',n,''//achar(27)//'[0m'
      call MPI_Abort(MPI_COMM_WORLD,1, error)
    end if
    if(sum3 .ne. n + 3) then
      print *, ''//achar(27)//'[31;1mFAILED !!! sum3 =', sum3, 'n =',n,''//achar(27)//'[0m'
      call MPI_Abort(MPI_COMM_WORLD,1, error)
    end if
    do i = 1, 10
    if(z(i) .ne. n*i +0) then
      print *, ''//achar(27)//'[31;1mFAILED !!! i =', i, 'z(i) =',z(i),'expected',n*i,''//achar(27)//'[0m'
      call MPI_Abort(MPI_COMM_WORLD,1, error)
    end if
    if(y(i) .ne. n*i +0) then
      print *, ''//achar(27)//'[31;1mFAILED !!! i =', i, 'y(i) =',y(i),'expected',n*i,''//achar(27)//'[0m'
      call MPI_Abort(MPI_COMM_WORLD,1, error)
    end if
    end do

    return
    end
