; wozmon.asm
;
; Originally from Apple-1 Operation Manual, Steve Wozniak, 1976
; Revised 2024 May 7 for custom 6502/MC6850 system by David R. Van Wagner davevw.com
; * Using MC6850 UART (instead of MC6520 and KBD/CRT)
; * extra processing for expected mark parity, software caps lock, and revised newline/carriage return processing
; * revised to expect terminal line edit mode instead of echo off character processing
; * revised to acme syntax

xaml=$24
xamh=$25
stl=$26
sth=$27
l=$28
h=$29
ysav=$2A
mode=$2B
in=$200

// MC6850
UART_DATA=$FFF8
UART_STCR=$FFF9

* = $F000
NMI: rti

;** MC6850 added by David R. Van Wagner davevw.com ***************************************
* = $FE00
UART_INIT:
    lda #0b00000011 ; 11=reset device
    sta UART_STCR
    lda #0b00001010 ; 0=rint disabled, 00=rtsn low, tint disabled 010=7e1 10=div 64
    sta UART_STCR
	rts
UART_OUT:
	pha
-	lda UART_STCR
	and #2
	beq - ; branch if TDRE=0, not finished transmitting
	pla
	sta UART_DATA
	cmp #13
	bne +
	lda #10 ; add newline along with carriage return
	bne UART_OUT
+	rts
UART_IN:
-	lda UART_STCR
	and #1
	beq - ; branch if TDRF=0, not received
	lda UART_DATA
	; software "CAPS LOCK" because wozmon expects only uppercase
	cmp #$61
	bcc +
	cmp #$78
	bcs +
	eor #$20
+   ora #$80 ; Apple Model 1 expects 7-bit with marked parity (8th bit always set)
	rts
;** MC6850 added by David R. Van Wagner davevw.com ***************************************

* = $FF00
RESET:
	cld
	cli
	jsr UART_INIT
	jmp escape
	nop
	nop
	nop
	nop
	nop
	nop
	nop
notcr:
	cmp #$94
	beq backspace
	cmp #$83
	beq escape
	iny
	bpl nextchar
escape:
	lda #$DC ; backslash
	jsr UART_OUT
getline:
	lda #13
	jsr UART_OUT
	ldy #1
backspace:
	dey
	bmi getline
nextchar:
	jsr UART_IN
	sta in, y
	;jsr UART_OUT - needed only if terminal echo off, line editing off
	cmp #$8A
	bne notcr
	ldy #$ff
	lda #$00
	tax
setstor:
	asl
setmode:
	sta mode
blskip:
	iny
nextitem:
	lda in, y
	cmp #$8A
	beq getline
	cmp #$AE ; period
	bcc blskip
	beq setmode
	cmp #$BA ; colon
	beq setstor
	cmp #$D2 ; R
	beq run
	stx l
	stx h
	sty ysav
nexthex:
	lda in, y
	eor #$B0
	cmp #$0A
	bcc dig
	adc #$88
	cmp #$FA
	bcc nothex
dig:
	asl
	asl
	asl
	asl
	ldx #4
hexshift:
	asl
	rol l
	rol h
	dex
	bne hexshift
	iny
	bne nexthex
nothex:
	cpy ysav
	beq escape
	bit mode
	bvc notstor
	lda l
	sta (stl, x)
	inc stl
	bne nextitem
	inc sth
tonextitem:
	jmp nextitem
run:
	jmp (xaml)
notstor:
	bmi xamnext
	ldx #2
setadr:
	lda l-1,x
	sta stl-1,x
	sta xaml-1,x
	dex
	bne setadr
nxtprnt:
	bne prdata
	lda #13
	jsr UART_OUT
	lda xamh
	jsr prbyte
	lda xaml
	jsr prbyte
	lda #$BA ; colon
	jsr UART_OUT
prdata:
	lda #32
	jsr UART_OUT
	lda (xaml,x)
	jsr prbyte
xamnext:
	stx mode
	lda xaml
	cmp l
	lda xamh
	sbc h
	bcs tonextitem
	inc xaml
	bne mod8chk
	inc xamh
mod8chk:
	lda xaml
	and #7
	bpl nxtprnt ; should always branch

*=$FFDC

prbyte:
	pha
	lsr
	lsr
	lsr
	lsr
	jsr prhex
	pla
prhex:
	and #$0F
	ora #$B0
	cmp #$BA
	bcc echo
	adc #6
echo:
	jmp UART_OUT

* = $fffa
	!byte <NMI, >NMI
	!byte <RESET, >RESET
	!byte $00, $00 ; IRQ
