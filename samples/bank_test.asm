; bank_test.asm
;

start=$c000 ; machine language org
chrout=$ffd2

; "hires ml"
* = start
        sei
        jsr disp_header

        jsr bank_norm
        lda #$A0
        sta $A000
        lda #$C8
        sta $C800
        lda #$0E
        sta $D800
        lda #$E0
        sta $E000

        jsr bank_ram
        lda #$D8
        sta $D800

        jsr bank_norm

        lda #0
        sta my_bank
-       jsr disp_test
        inc my_bank
        lda #$07
        bit my_bank
        beq +
        bne -        

+       jsr bank_norm
        cli
        rts

bank_norm
        lda $01
        ora #$07
        sta $01
        rts

bank_ram
        lda $01
        and #$f8
        sta $01
        rts

bank_select
        lda $01
        and #$f8
        ora my_bank
        sta $01
        rts

disp_test
        lda #' '
        jsr chrout
        lda my_bank        
        jsr disp_hex
        lda #$00
        ldx #$A0
        jsr disp_banked
        lda #$00
        ldx #$C8
        jsr disp_banked
        lda #$00
        ldx #$D8
        jsr disp_banked
        lda #$00
        ldx #$E0
        jsr disp_banked
        lda #13
        jsr chrout
        rts

disp_banked
        sta $FB
        stx $FC

        lda #' '
        jsr chrout
        jsr chrout
        jsr chrout

        ldy #0
        jsr bank_select
        lda ($FB),Y
        pha
        jsr bank_norm
        pla
        jsr disp_hex

        rts

disp_hex
        pha
        lsr
        lsr
        lsr
        lsr
        jsr disp_digit
        pla
        and #$0f
        ; fall thru disp_digit

disp_digit
        cmp #16
        bcs ++
        cmp #10
        bcs +
        adc #'0'
        jmp chrout
+       sbc #$0A
        adc #$40
        jmp chrout
++      rts        

disp_header
        ldy #0
-       lda header,y
        beq +
        jsr chrout
        iny
        bne -
+       rts        

header 
        !text "BANK A000 C800 D800 E000"
        !byte 13 ; carriage return
        !byte 0 ; end of string

my_bank !byte 0

