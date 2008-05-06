/ maki -- make initialization dec tape

 sys open; std0; 0
 bes error
 mov r0,r1
 sys read; buf; 512.
 mov r1,r0
 sys close
 sys open; tap0; 1
 bes error
 mov r0,r1
 sys write; buf; 512.
 bes error
 sys open; rf; 0
 bes error
 mov r0,r2
 jsr pc,copy
 mov r1,r0
 sys close
 sys open; tap1; 1
 bes error
 mov r0,r1
 jsr pc,copy
 sys exit

copy:
 mov $512.,-(sp)
1:
 mov r2,r0
 sys read; buf; 512.
 bes error
 mov r1,r0
 sys write; buf; 512.
 bes error
 dec (sp)
 bne 1b
 tst (sp)+
 rts pc

error:
 mov $1,r0
 sys write; mes; emes-mes
 sys exit

mes:
<error in copy\n>
emes:
tap0: </dev/tap0\0>
tap1: </dev/tap1\0>
rf: </dev/rf0\0>
std0: </etc/std0\0>
.even

buf:	.=.+512.
