;
; DZ80.CTL - Control File for DZ80 Disassembler V 3.4.1; vim: set syntax=asm:
;
;   Control codes allowed in the CTL file:
;
;  A - Address		Specifies that the word entry is the address of
;			something for which a label should be generated.
;
;  B - Byte binary	Eight bit binary data (db).
;
;  C - Code		Executable code that must be analyzed.
;
;  I - Ignore		Treat as uninitialized space. Will not be dis-
;			assembled as anything unless some other valid
;			code reaches it.
;
;  L - Label		Generate a label for this address.
;
;  N - No label		Suppress label generation for an address.
;
;  O - Offset		Specify offset to add to addresses.
;
;  P - Patch		Add inline code (macro, for example)
;
;  S - Symbol		Generate a symbol for this value.
;
;  T - Text		ASCII text (db).
;
;  W - Word binary	Sixteen bit binary data (dw).
;
;  X - Operand name	Specify special name for operand.
;
;  Y - Operand name	Specify special name for operand, suppress EQU generation.
;
;  # - Comment		Add header comment to output file.
;
;  ! - Inline comment	Add comment to end of line.
;
;  The difference between labels and symbols is that a label refers
;  to a referenced address, whereas a symbol may be used for 8 or 16
;  bit immediate data. For some opcodes (eg: ld a,xx) only the symbol
;  table will be searched. Other opcodes (eg: ld hl,xx) will search
;  the label table first and then search the symbol table only if the
;  value is not found in the label table.
;
;  Values may specify ranges, ie:
;
;	A 100-11f specifies a table of addresses starting at address
;		0x100 and continuing to address 0x11f.
;
;	T 100-120 specifies that addresses 0x100 through (and including)
;		address 0x120 contain ascii text data.
;
;  Range specifications are allowed for codes A, B, C, I, T, and W, but
;  obviously don't apply to codes L and S.
;
;---------------------------------------------------------------------------
;

N 0FFF
S 0FFF 0fffh
D 00D2 2

N 1000
S 1000 1000h
D 02D1 2
D 14F1 2
D 1A6A 2
D 1C72 2

L 0000 boot
S 0000 0
D 0A9B 1
D 1848 1
D 1913 1
D 1A3C 1

N 000d
S 000d 0dh
D 1636 2

N 0011
S 0011 11h
D 029B 2

N 0014
S 0014 14h
D 1EAB 2

N 00ff
S 00ff 0ffh
D 181F 2

N ff01
S ff01 0ff01h
D 1818 2


; This label gets broken.
L 2000 rambase
P 0000 rambase equ     02000h

; Gaps
i 0007
i 000e-000f
i 0015-0017
i 0024-0027
i 002c-002f
i 1fd7-1fff

; Binary tables and data. (Some of these are 'textish' but 't' breaks them.)
b 02d9-02ea
b 0476-04ce
a 08e5-08fa
b 0ad9-0b1e
b 17ec-17fd
b 17fe-1814
b 1cad-1ccc

; Normal labels
L 0008 ivK
L 0010 ivR
L 0018 ivKR
L 0020 ivV
L 0030 ivVR
L 0038 ivVRK
L 003b init
L 0049 csrom
L 00a1 memory_error
L 00a8 memory_ok

L 00fd in_kb
L 038B memmov
L 03cc in_rcv
L 0476 KBTab

L 1083 memset
L 14d4 toggle_cursor
L 1cad lSETUP
L 1cb6 lEXIT

; Data segment labels.
L 2068 KBflg
L 206A KBnbuf

; Comments from haxrom
P 0000 ; Interrupt vector begins at 0
P 0000 ;;; IV 0x00: boot
P 0008 ;;; IV 0x08: kbd
P 0010 ;;; IV 0x10: recv
P 0018 ;;; IV 0x18: recv & kbd
P 0020 ;;; IV 0x20: vertical freq.
P 0028 ;;; IV 0x28: vertical freq. & kbd (kbd ignored)
P 0030 ;;; IV 0x30: vertical freq. & recv
P 0038 ;;; IV 0x38: vertical freq. & recv & kbd (kbd ignored)

P 0080 ;;; Zero all RAM. C gets the high byte of the address of the top of
P 0080 ;;; RAM, HL is loaded with it, and it zeros memory all the way down,
P 0080 ;;; leaving the start of RAM in HL
P 0080 ;;;
P 0080 ;;; From start of RAM, test that it's all zero

P 00fd ;;; Keyboard interrupt handler

P 0131 ;;; End of keyboard interrupt handler
P 0131

P 02a4 ;;; Zero all scratch and screen RAM above stack


P 02d9 ;;; 0x02d9 through 0x02eb are the initial values for the screen RAM;
P 02d9 ;;; it's copied into low RAM at 0x2000 during init.
P 02eb ;;; Initialize several scratch values. We don't know what all of these are for yet.
P 038b ;;; Performs a memmove(HL,DE,B)

P 03cc ;;; Receiver interrupt handler
P 043e ;;; End of keyboard interrupt handler

; P 04CF ;;; Vertical interrupt handler
P 1083 ;;; memset: Set memory from HL to HL+DE-1 to value B
P 195D ;;; Now copy from 21d3 to nvr latch, one per edge, 21 bytes

! 0001 Initialize SP to top of stack
! 0008 call keyboard handler
! 0010 call receiver handler
! 0018 call receiver handler
! 001B call keyboard handler
! 0020 call vertical handler
! 0028 call vertical handler
! 0030 call receiver handler
! 0033 call vertical handler
! 003B e <- 1
! 0040 0x0f to NVR latch
! 0042 invert A (0xf0)
! 0043 0xf0 to brightness
! 0045 zero A
! 0046 zero D, L, H

! 0049 next 2K ROM
! 004A store the current ROM # in B
! 004B show current ROM on LEDs
! 004D Checksum over 8 256B blocks
! 004F    Rotate A left
! 0050    A <- A ^ memory
! 0051    next memory address
! 0052    ... repeat until end of block
! 0055 next block
! 0056 decrement block count
! 0057 ... repeat until end of chip
! 005A Check the value of the accumulator
! 005B If it's not zero, hang forever
! 005E Put the ROM # back in the accumulator
! 005F If it's not 4...
! 0061 repeat for the next ROM
! 0065 write 5 to LEDs

! 0067 Test pattern: 0xaa
! 0069 0x2c is the top of RAM on the basic board
! 006B 
! 006D flags buffer &= 0x02 : is AVO installed?
! 006F if bit 2 is not set, AVO is installed
! 0072 0x40 is the top of RAM on the AVO
! 0074 load HL with top of RAM
! 0075 dec HL
! 0076 M <- 0
! 0078 dec HL
! 0079 a <- h
! 007A compare to 1f (top of ROM)
! 007C continue to zero if not at ROM

! 0080 read a byte of memory
! 0082 if it's zero, skip ahead
! 0090 run test pattern through ram

! 00a5 Hang if non-AVO memory
! 00B3 Repeat mem test with pattern 0x55

! 00BA zero scratch and screen mem
! 00BD Set a number of scratch values to fixed values
! 00C0 .... more memory initialization
! 00FE Read from keyboard port
! 0102 copy b <- a
! 0103 subtract 0x7c from a
! 0105 jump to 011a if a is a normal key
! 0108 otherwise h <- a
! 0109 h++
! 010A a <- 0x10
! 0112 load keys flag addresss
! 0115 or in control bit
! 0116 store again
! 0117 jump to end to interrupt
! 011A load keys flag address
! 011F load key counter
! 0120 if overflow.. (>3 keys)
! 0122 discard keypress and end interrupt
! 0125 otherwise increment key counter
! 0126 load key address buffer?
! 0129 add key counter to address buffer
! 012C save key to address buffer
! 02A4 HL <- 0x204e (top of stack)
! 02A7 DE <- 0x0fb2
! 02AA B  <- 0
! 02AC memset(0x204e,0,0xfb2)
! 02AF invert A
! 02B0 store A in 0x2104
! 02B3 HL <- 0x2004
! 02B6 *(0x2052) = 0x2004
! 02B9 HL <- 0x22d0
! 02BC *(20f6) = 0x22d0
! 02C0 0x2140 = 0xe605
! 02C3 HL = 0x2000
! 02C6 DE = 0x02d9
! 02C9 B = 18
! 02CB copy 18 bytes from 0x02d9 to 0x2000
! 02CE HL = 0x3000
! 02D1 DE = 0x1000
! 02D4 B = 0xFF
! 02D6 Set all of attribute RAM to 0xFF and return
! 02EB HL = 0x0212
! 02EE Store 0x1202 at 0x212d
! 02F1 A = 0x35
! 02F3 0x212c = 0x35
! 02F6 A = 1
! 02F8 0x205b = 1
! 02FB 0x2176 = 1
! 02FE HL = 0x07ff
! 0301 store 0xff07 at 0x2149
! 0304 A = 2
! 0306 0x2073 = 2
! 0309 A = 0xf7
! 030B 0x20fa = 0xf7
! 030E read flags buffer
! 0310 And it with 0x04 (check graphics flag)
! 0312 A=1
! 0314 if no graphics skip
! 0317 else store 1 in 0x2079
! 031A A = 0xff
! 031C 0x210e = 0xff
! 031F 0x21ba = 0xff
! 0322 H = 0x80
! 0324 L = 0x80
! 0325 Store 0x8080 at 0x20c0
! 038B Load memory at DE into accumulator
! 038C Store it at HL
! 038D HL++
! 038E DE++
! 038F B--
! 0390 until B is zero
! 03CF Read from PUSART data
! 03DE Read from PUSART ctrl
! 04CE Potential Free
! 08D4 subtract 5 from A and return if negative
! 08D7 load jump table at 8e5
! 08DA double a
! 08DE hl = X08e5 + 2A
! 08DF load DE from (HL) little-endian
! 08E3 zero A
! 08E4 jump to address
! 08E7 immediate return
! 08ED MYSTERY ADDRESS: this isn't even in AVO!!!
! 0A15 HL = 0x05e6
! 0A18 0x2140 = 0xe605
! 1083 M <- B
! 1084 HL++
! 1085 DE--
! 1087 A = D | E
! 1088 repeat if A != 0
! 1493 Read flags buffer
! 1495 check if transmit buffer is empty
! 1497 if zero, return
! 1498 HL = 0x2144
! 149B Load from 2144
! 149C Check if zero
! 149D
! 14A0 If not, A = 10 (else A is zero)
! 14A5 or bits from 0x21a5
! 14A9 or bits from 0x2145
! 14AA
! 14AB ... and 0x2146
! 14AC
! 14AD ... and 0x2147
! 14AE zero 0x2147
! 14B0
! 14B1 ... and 0x2148
! 14B2 zero 0x2148
! 14B4 Write to keyboard buffer
! 14B6
! 14B9 Increment what's at 0x2074
! 14BA load what's at 0x2077
! 14BD return if it's not zero
! 14BF A =  0x2065
! 14C5 return if [0x2065] | [0x2051] != 0
! 175B Initialize start of screen RAM and wipe attribute RAM to 0xff
! 175E B = 0x0a
! 1760 D = 1
! 1762 push B, D
! 1764 Display wait message
! 1767 pop D
! 1768 disable interrupts(???)
! 1769 HL = 0x217b
! 176C E = 0x33
! 176E C = 1
! 1770 A = 0
! 1771 0x21ae = A
! 1774 A = C
! 1775 0x21ad = A
! 1778 push D,H
! 177A A=D
! 177B or A
! 177C load A from HL
! 177D store it in 0x21af
! 1780 if A is 0 call X18ae
! 1783 call X18a3
! 1786 pop H,D
! 17BE DE = 0x17ec
! 17C1 B = 7
! 17C3 HL = 0x21cc
! 17C6 Initialize 0x21cc to Wait message
! 17C9 HL = 0xcc71
! 17CC retarget screen ram to display wait message
! 17DC DE = 0x17f3
! 17DF B = 11
! 17E1 store
! 18CD Write READ command to NVR latch
! 18DD Write 0x2fh (standby) to nvr latch
! 18E1 HL=X21d3
! 18E4 B= 14 -- we're reading 14 bits
! 18EF wait for NVR clock L
! 18F2 shft bit out of nvr
! 18F9 wait for NVR clock H
! 18FC read bit and store it in memory
! 1903 wait for NVR clock L
! 1907 read next bit
! 190A send standby
! 190E DE=0x21d3
! 1911 B = 14
! 1913 HL = 0
! 1917 load next char
! 1918 and with NVR data bit
! 191A rotate NVR bit to high bit
! 1924 Store the finished data in 21af and 21ae
! 1928 Load accumulator from 0x21ae -- (0x42)
! 192B B = 0xff
! 192D increment B
! 192E subtract 0x0a from accumulator
! 1930 repeat while accumulator is positive
! 1933 add 0x0a to accumulator
! 1935 HL = 0x21d3
! 1938 E = 0x23
! 193A D = 0x14
! 193C
! 193D HL++
! 193E D--
! 193F repeat until D is zero
! 1942 Store 0x2f in 21e7
! 1944 HL = 0x21d3
! 1947 copy A to E
! 1948 clear D
! 194A add DE to HL -- calculate address of bit A
! 194B Store 0x22 in M
! 194D HL = 0x21d3
! 1950 A = 0x0a
! 1952 A = 0x0a + B
! 1964 Flags Buffer & C
! 1969 Return if B is negative


