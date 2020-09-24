subroutine WS_ASSERT(a,b)
  integer a,b
  if (a .ne. b) then
    write(*,*) 'FAILED !'
    call exit(1)
  endif
  end

  program main
    implicit none
    integer x
    common/x/ x

    integer ( kind = 4 ) i,j,sum,sum2
    x=0 
    !$ws stopsteal
    call WS_ASSERT(x,1)
    !$ws resteal
    call WS_ASSERT(x,2)
    !$ws do reduction(+:sum) private(j) steal_schedule(guided,5) schedule(dynamic,1)
    do i = 1,10
    sum=sum+1 
    end do
    call WS_ASSERT(x,3)

    !$ws critical
    sum2 = sum2 + 1
    !$ws end critical
    call WS_ASSERT(x,4)
    write(*,*) 'SUCCESS !' 
    end
