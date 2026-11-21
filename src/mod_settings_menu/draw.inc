.global p3p_config_draw_hook
p3p_config_draw_hook:
    lw $t0, CONFIG_VARIANT($a1)
    bnez $t0, .Lconfig_draw
    lui $t0, %hi(menu_state)
    lw $t0, %lo(menu_state)($t0)
    beqz $t0, .Lconfig_draw
    nop

    addiu $sp, $sp, -32
    sw $s3, 8($sp)
    sw $s4, 12($sp)
    sw $s5, 24($sp)
    sw $ra, 28($sp)
    lbu $t1, CONFIG_PALETTE_INDEX($a1)
    xori $t1, $t1, 1            # Native palette 1 is the female route.
    sltiu $t1, $t1, 1
    sll $t1, $t1, 5             # Each palette contains eight 32-bit colors.
    lui $s5, %hi(menu_palettes)
    addiu $s5, $s5, %lo(menu_palettes)
    addu $s5, $s5, $t1
    li $t1, MENU_OPEN
    li $s3, 255                 # Opacity for every label in this frame.
    beq $t0, $t1, .Ldraw_contents
    move $s4, $zero             # Vertical text offset.

    sw $a0, 16($sp)
    sw $a1, 20($sp)
    jal game_time_us
    nop
    lui $t0, %hi(menu_started_us)
    lw $t0, %lo(menu_started_us)($t0)
    subu $t0, $v0, $t0         # Unsigned elapsed time also handles timer wrap.
    lui $t2, %hi(menu_state)
    lw $t3, %lo(menu_state)($t2)
    li $t1, MENU_OPENING
    li $t4, OPEN_TIME_US
    bne $t3, $t1, .Lfade_out
    move $t1, $t4
    sltu $t3, $t0, $t1
    beqz $t3, .Lfinish_opening
    li $t3, 255
    mul $s3, $t0, $t3
    divu $zero, $s3, $t1
    mflo $s3
    li $s4, 256
    subu $s4, $s4, $s3
    b .Ldraw_contents
    srl $s4, $s4, 6
.Lfinish_opening:
    li $t3, MENU_OPEN
    b .Ldraw_contents
    sw $t3, %lo(menu_state)($t2)

.Lfade_out:
    li $t1, CLOSE_TIME_US
    sltu $t3, $t0, $t1
    beqz $t3, .Lfinish_closing
    subu $t0, $t1, $t0
    li $t3, 255
    mul $s3, $t0, $t3
    divu $zero, $s3, $t1
    b .Ldraw_contents
    mflo $s3
.Lfinish_closing:
    sw $zero, %lo(menu_state)($t2)
    lw $a0, 16($sp)
    lw $a1, 20($sp)
    lw $ra, 28($sp)
    lw $s3, 8($sp)
    lw $s4, 12($sp)
    lw $s5, 24($sp)
    b .Lconfig_draw
    addiu $sp, $sp, 32

.Ldraw_contents:
    beqz $s3, .Ldraw_return
    nop
    lui $a3, %hi(menu_title)
    addiu $a3, $a3, %lo(menu_title)
    lw $a0, PALETTE_TITLE($s5)
    li $t2, LABEL_X
    li $t3, 36
    jal .Ldraw_text
    nop

    lui $a3, %hi(xp_label)
    addiu $a3, $a3, %lo(xp_label)
    move $a0, $zero
    jal .Ldraw_row
    li $a1, 62

    lui $a3, %hi(money_label)
    addiu $a3, $a3, %lo(money_label)
    li $a0, 1
    jal .Ldraw_row
    li $a1, 86

    lui $a3, %hi(community_label)
    addiu $a3, $a3, %lo(community_label)
    li $a0, COMMUNITY_ROW
    jal .Ldraw_row
    li $a1, 110

    lui $a3, %hi(shrine_label)
    addiu $a3, $a3, %lo(shrine_label)
    li $a0, SHRINE_ROW
    jal .Ldraw_row
    li $a1, 134

    lui $a3, %hi(shrine_selection_label)
    addiu $a3, $a3, %lo(shrine_selection_label)
    li $a0, SHRINE_SELECTION_ROW
    jal .Ldraw_row
    li $a1, 158

    lui $a3, %hi(battle_drop_label)
    addiu $a3, $a3, %lo(battle_drop_label)
    li $a0, BATTLE_DROP_ROW
    jal .Ldraw_row
    li $a1, 182

    lui $a3, %hi(bench_xp_label)
    addiu $a3, $a3, %lo(bench_xp_label)
    li $a0, BENCH_XP_ROW
    jal .Ldraw_row
    li $a1, 206

    lui $t0, %hi(menu_row)
    lw $t0, %lo(menu_row)($t0)
    lui $a3, %hi(xp_description)
    addiu $a3, $a3, %lo(xp_description)
    lui $t1, %hi(money_description)
    addiu $t1, $t1, %lo(money_description)
    movn $a3, $t1, $t0
    lui $t1, %hi(community_description)
    addiu $t1, $t1, %lo(community_description)
    xori $t2, $t0, COMMUNITY_ROW
    movz $a3, $t1, $t2
    lui $t1, %hi(shrine_description)
    addiu $t1, $t1, %lo(shrine_description)
    xori $t2, $t0, SHRINE_ROW
    movz $a3, $t1, $t2
    lui $t1, %hi(shrine_selection_description)
    addiu $t1, $t1, %lo(shrine_selection_description)
    xori $t2, $t0, SHRINE_SELECTION_ROW
    movz $a3, $t1, $t2
    lui $t1, %hi(battle_drop_description)
    addiu $t1, $t1, %lo(battle_drop_description)
    xori $t2, $t0, BATTLE_DROP_ROW
    movz $a3, $t1, $t2
    lui $t1, %hi(bench_xp_description)
    addiu $t1, $t1, %lo(bench_xp_description)
    xori $t0, $t0, BENCH_XP_ROW
    movz $a3, $t1, $t0
    lw $a0, PALETTE_DESCRIPTION($s5)
    li $t2, LABEL_X
    jal .Ldraw_text
    li $t3, 236

.Ldraw_return:
    lw $ra, 28($sp)
    lw $s3, 8($sp)
    lw $s4, 12($sp)
    lw $s5, 24($sp)
    jr $ra
    addiu $sp, $sp, 32
.Lconfig_draw:
    addiu $sp, $sp, -160
    j game_config_draw_continue
    sw $s2, 72($sp)
