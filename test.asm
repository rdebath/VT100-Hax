; test file
LAB1	EQU	$
;
START	NOP		; first line
	JMP	START	; line 2
LAB2	EQU	$
STORAGE	DAC	$
STRING	DB	"This is a test"
BYTES	DB	0x34,0x93,120,17,0x0d
CMD	DB	"TEST",59
	DW	START
	DB	"LARGE",17
	DW	$+9
TABLE	DW	$, STORAGE, STRING
	END
