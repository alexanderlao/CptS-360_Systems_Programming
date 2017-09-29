.global main, mymain, myprintf

main:
	pushl %ebp		# save the current value of ebp
	movl %esp, %ebp		# copy esp to ebp
	
	subl $12, %esp		# make room on the stack

	pushl 16(%ebp)		# saving env[] on the stack
	pushl 12(%ebp)		# saving argv[] on the stack
	pushl 8(%ebp)		# saving argc on the stack
	
	call mymain

	addl $16, %esp		# tearing down the stack

	movl %ebp, %esp
	popl %ebp
	ret
