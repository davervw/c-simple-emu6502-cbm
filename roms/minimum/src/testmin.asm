; testmin.asm
; Copyright (c) 2024 by David R. Van Wagner
; davevw.com
;
; test 6502/6850 minimum system operation
; echos characters, changing case of alphabetical entries

; MC6850
UART_DATA = $FFF8 ; data register
UART_STCR = $FFF9 ; status and control register

* = $F000
RESET:
    lda #$80
    ldx #$f0
    jsr STROUT
--  bit UART_STCR
    bpl --
    lda UART_DATA
    cmp #$41 ; 'A'
    bcc ++
    cmp #$5b ; 'Z'+1
    bcs +
    ora #$20 ; to lowercase
    bne ++
+   cmp #$61 ; 'a'
    bcc ++
    cmp #$7b ; 'z'+1
    bcs ++
    eor #$20 ; to uppercase
++
-   bit UART_STCR
    bvc -
    sta UART_DATA
    jmp --
STROUT:
    sta $00
    stx $01
    ldy #$00
--  lda ($00),y
    beq +
-   bit UART_STCR
    bvc -
    sta UART_DATA
    iny
    bne --
+   rts

* = $F080
    !text "HELLO MC6850 UART!", 13, 10, 0
    !text "*" ; test that STROUT finishes

* = $FFF0
NMI:rti
IRQ:jmp IRQ

* = $FFFA
    !byte <NMI, >NMI
    !byte <RESET, >RESET
    !byte <IRQ, >IRQ
