

;******************************************************************************
;******************************************************************************

VDU_MEM_START	equ	0CC00h
VDU_MEM_END	equ	0CFFFh


;******************************************************************************
;******************************************************************************

	org	0000h


;******************************************************************************
;******************************************************************************

	mvi	a, 30h
	call	vdu_mem_init
	ret


;******************************************************************************
;Routine name:	vdu_mem_init
;Description:	Initialize SOL-20 VDU memory.
;Created:	17 December 2011
;
;Parameters:	A:
;			Code to use to initialize VDU memory.
;
;Returns:	void
;******************************************************************************

vdu_mem_init:
	push	b
	push	h

	mov	b, a		;Save parameter.

	lxi	h, VDU_MEM_START

vdu_mem_init_00:
;	mvi	m, 41h		;Code to use to fill VDU Memory.
	mov	m, b		;Code to use to fill VDU Memory.
	inx	h		;Next VDU memory location.

	;Loop until (VDU memory + 1) is reached.
	;---------------------------------------
	mov	a,h
	cpi	(VDU_MEM_END + 1) >> 8
	jnz	vdu_mem_init_00
	mov	a,l
	cpi	(VDU_MEM_END + 1) & 00FFh
	jnz	vdu_mem_init_00

	pop	h
	pop	b

	ret


;******************************************************************************
;******************************************************************************

	end






