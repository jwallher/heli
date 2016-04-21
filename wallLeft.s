@ wallLeft.s
@function to set the wall to move
.global wallLeft
wallLeft:
	@r0 is curent x r1 is origx
	cmp r0, #0
	beq .moveBack
	sub r0, r0, #1
	b .done
	
	.moveBack:
	mov r0,r1
	b .done
	

	.done:
	mov pc, lr
