!MODULE arg
!use, intrinsic :: ISO_C_BINDING
!implicit none
!!:: x
!integer (c_int) :: x
!common x
!END MODULE arg
!function MPC_Workshare_stop_stealing()
!write(*,*)"BACHIBOUZOUK"
!x=x+1
!end
subroutine WS_ASSERT(a,b)
  integer a,b
  if (a .ne. b) then
    write(*,*) 'FAILED !'
    call exit(1)
  endif
  end

  program main
    !use arg
    implicit none
    integer x
    common/x/ x
    !integer::x
    !common x

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
