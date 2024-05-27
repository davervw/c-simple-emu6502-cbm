; testmin.asm
; Copyright (c) 2024 by David R. Van Wagner
; davevw.com
;
; test 6502/6850 minimum system operation
; echos characters, changing case of alphabetical entries

; MC6850
UART_DATA = $FFF8 ; data register (read/write)
UART_STCR = $FFF9 ; status (read) and control register (write)

* = $F000
RESET:
    lda #0b00000011 ; reset device
    sta UART_STCR
    lda #0b00010110 ; 0=rint disabled, 00=rtsn low, tint disabled 101=8n1 10=div 64
    sta UART_STCR
    lda #$80
    ldx #$f0
    jsr STROUT
--  lda UART_STCR
    and #1
    beq --
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
    tax
-   lda UART_STCR
    and #2
    beq - ; branch if TDRE-0 (not empty) waiting for transmit
    stx UART_DATA
    jmp --
STROUT:
    sta $00
    stx $01
    ldy #$00
--  lda ($00),y
    tax
    beq +
-   lda UART_STCR
    and #2
    beq - ; branch if TDRE=0 (not empty) waiting for transmit
    stx UART_DATA
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
