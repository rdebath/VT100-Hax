;******************************************************************************
;Filename:	test_solace.asm
;Description:	Program to Test the Solace (SOL-20 emulator).
;Copyrigth(c):
;Author(s):	Claude Sylvain
;Created:	December 2011
;Last modified:	28 December 2011
;
;Notes:		- Solace (SOL-20 emulator) is available at the following
;		  place:
;			  http://www.sol20.org/solace.html
;******************************************************************************


;******************************************************************************
;				      EQU
;******************************************************************************

VDU_MEM_START	equ	0CC00h		;VDU Memory Start.
VDU_MEM_END	equ	0CFFFh		;VDU Memory End (last location).


;******************************************************************************
;				      TEXT
;******************************************************************************

	org	0000h


;******************************************************************************
;Description:	The entry point.
;******************************************************************************

entry
	;Init. VDU.
	;----------
	mvi	a, 30h		;Init. VDU mem. with '0'.
	call	vdu_mem_init

	ret			;Return control to system.


;******************************************************************************
;Routine name:	vdu_mem_init
;Description:	Initialize SOL-20 VDU memory.
;Created:	17 December 2011
;Last modified:	28 December 2011
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
;	cpi	(VDU_MEM_END + 1) >> 8
	cpi	high (VDU_MEM_END + 1)
	jnz	vdu_mem_init_00
	mov	a,l
;	cpi	(VDU_MEM_END + 1) & 00FFh	;No more supported by asm8080 V1.01
;	cpi	(VDU_MEM_END + 1) and 00FFh
	cpi	low (VDU_MEM_END + 1)
	jnz	vdu_mem_init_00

	pop	h
	pop	b

	ret


;******************************************************************************
;******************************************************************************

	end





