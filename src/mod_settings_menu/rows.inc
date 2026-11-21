# a0 selects the row, a1 is y, a3 points to the label.
.Ldraw_row:
    addiu $sp, $sp, -32
    sw $ra, 28($sp)
    sw $a0, 20($sp)
    sw $a1, 24($sp)
    sw $a3, 12($sp)
    lui $t0, %hi(menu_row)
    lw $t0, %lo(menu_row)($t0)
    xor $t0, $t0, $a0
    lw $a0, PALETTE_IDLE_ROW($s5)
    lw $t1, PALETTE_SELECTED_ROW($s5)
    movz $a0, $t1, $t0
    lw $t1, PALETTE_ROW($s5)
    lw $t2, PALETTE_SELECTED_TEXT($s5)
    movz $t1, $t2, $t0
    sw $t1, 16($sp)
    addiu $a2, $a1, -3
    li $a1, ROW_X
    jal .Ldraw_rect
    li $a3, ROW_WIDTH

    lw $t0, 20($sp)
    lui $t1, %hi(p3p_xp_mode)
    addiu $t1, $t1, %lo(p3p_xp_mode)
    sll $t2, $t0, 2
    addu $t1, $t1, $t2
    lw $t1, 0($t1)
    sw $t1, 8($sp)

    # All values use the same right-hand cell.
    lw $a0, PALETTE_SELECTED_ROW($s5)
    li $a1, VALUE_X
    lw $a2, 24($sp)
    addiu $a2, $a2, -3
    jal .Ldraw_rect
    li $a3, VALUE_WIDTH

    lw $a3, 12($sp)
    lw $a0, 16($sp)
    lw $t3, 24($sp)
    jal .Ldraw_text
    li $t2, LABEL_X

    lw $t0, 20($sp)
    lw $t1, 8($sp)
    sltiu $t2, $t0, COMMUNITY_ROW
    beqz $t2, .Ldraw_toggle
    nop
    sltiu $t0, $t1, 5
    movz $t1, $zero, $t0
    sll $t1, $t1, 3
    lui $a3, %hi(mode_labels)
    addiu $a3, $a3, %lo(mode_labels)
    addu $a3, $a3, $t1
    lw $a0, PALETTE_VALUE($s5)
    b .Ldraw_value
    li $t2, VALUE_X + 16

.Ldraw_toggle:
    # Off is the right half. On is the left half.
    sltiu $t1, $t1, 1
    sw $t1, 8($sp)
    li $t2, VALUE_WIDTH / 2
    movz $t2, $zero, $t1
    addiu $a1, $t2, VALUE_X
    lw $a2, 24($sp)
    addiu $a2, $a2, -3
    lw $a0, PALETTE_VALUE($s5)
    jal .Ldraw_rect
    li $a3, VALUE_WIDTH / 2

    lui $a3, %hi(toggle_labels)
    addiu $a3, $a3, %lo(toggle_labels)
    lw $t1, 8($sp)
    lw $a0, PALETTE_SELECTED_ROW($s5)
    lw $t0, PALETTE_MUTED_VALUE($s5)
    movn $a0, $t0, $t1
    lw $t3, 24($sp)
    jal .Ldraw_text
    li $t2, VALUE_X + 7

    lui $a3, %hi(toggle_labels + 4)
    addiu $a3, $a3, %lo(toggle_labels + 4)
    lw $t1, 8($sp)
    lw $a0, PALETTE_SELECTED_ROW($s5)
    lw $t0, PALETTE_MUTED_VALUE($s5)
    movz $a0, $t0, $t1
    li $t2, VALUE_X + VALUE_WIDTH / 2 + 5
.Ldraw_value:
    lw $t3, 24($sp)
    lw $ra, 28($sp)
    b .Ldraw_text
    addiu $sp, $sp, 32

# a0 is RRGGBBAA, a1/a2 are x/y, a3 is width in pixels.
.Ldraw_rect:
    addiu $sp, $sp, -32
    sw $ra, 28($sp)
    ins $a0, $s3, 0, 8
    wsbw $t0, $a0
    sw $t0, 0($sp)
    addu $a2, $a2, $s4
    mtc1 $a1, $f0
    mtc1 $a2, $f1
    mtc1 $a3, $f2
    cvt.s.w $f0, $f0
    cvt.s.w $f1, $f1
    cvt.s.w $f2, $f2
    swc1 $f0, 4($sp)
    swc1 $f1, 8($sp)
    swc1 $f2, 12($sp)
    li $t0, 0x41b00000         # 22.0 px row height.
    jal game_prepare_rect
    sw $t0, 16($sp)
    move $a0, $sp
    addiu $a1, $sp, 4
    move $a2, $zero
    jal game_draw_rect
    mtc1 $zero, $f12
    lw $ra, 28($sp)
    jr $ra
    addiu $sp, $sp, 32

.Ldraw_text:
    ins $a0, $s3, 0, 8
    addu $t3, $t3, $s4
    mtc1 $t2, $f12
    mtc1 $t3, $f13
    cvt.s.w $f12, $f12
    cvt.s.w $f13, $f13
    mtc1 $zero, $f14
    move $a1, $zero
    li $a2, 1
    move $t0, $zero
    j game_draw_text
    move $t1, $zero
