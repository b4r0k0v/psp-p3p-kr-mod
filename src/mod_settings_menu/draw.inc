.global p3p_config_draw_hook
p3p_config_draw_hook:
    lw $t0, CONFIG_VARIANT($a1)
    bnez $t0, .Lconfig_draw
    lui $t0, %hi(menu_state)
    lw $t0, %lo(menu_state)($t0)
    beqz $t0, .Lconfig_draw_with_hint
    nop

    addiu $sp, $sp, -48
    sw $s0, 32($sp)
    sw $s1, 36($sp)
    sw $s2, 40($sp)
    sw $s3, 8($sp)
    sw $s4, 12($sp)
    sw $s5, 24($sp)
    sw $ra, 44($sp)
    sw $a1, 20($sp)
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
    lw $ra, 44($sp)
    lw $s0, 32($sp)
    lw $s1, 36($sp)
    lw $s2, 40($sp)
    lw $s3, 8($sp)
    lw $s4, 12($sp)
    lw $s5, 24($sp)
    b .Lconfig_draw
    addiu $sp, $sp, 48

.Ldraw_contents:
    beqz $s3, .Ldraw_return
    nop
    lui $a3, %hi(menu_title)
    addiu $a3, $a3, %lo(menu_title)
    lw $a0, PALETTE_TITLE($s5)
    li $t2, LABEL_X
    li $t3, 46
    jal .Ldraw_text
    nop

    lui $t0, %hi(menu_row)
    lw $t0, %lo(menu_row)($t0)
    addiu $t0, $t0, '1'
    sb $t0, 0($sp)
    li $t0, '/'
    sb $t0, 1($sp)
    li $t0, '0' + ROW_COUNT
    sb $t0, 2($sp)
    sb $zero, 3($sp)
    move $a3, $sp
    lw $a0, PALETTE_TITLE($s5)
    li $t2, VALUE_X + VALUE_WIDTH - 30
    jal .Ldraw_text
    li $t3, 46

    lui $s0, %hi(menu_scroll)
    lw $s0, %lo(menu_scroll)($s0)
    addiu $s1, $s0, VISIBLE_ROWS
    li $s2, 76
.Ldraw_row_loop:
    lui $a3, %hi(difficulty_label)
    addiu $a3, $a3, %lo(difficulty_label)
    lui $t1, %hi(xp_label)
    addiu $t1, $t1, %lo(xp_label)
    xori $t2, $s0, XP_ROW
    movz $a3, $t1, $t2
    lui $t1, %hi(money_label)
    addiu $t1, $t1, %lo(money_label)
    xori $t2, $s0, MONEY_ROW
    movz $a3, $t1, $t2
    lui $t1, %hi(bench_xp_label)
    addiu $t1, $t1, %lo(bench_xp_label)
    xori $t2, $s0, BENCH_XP_ROW
    movz $a3, $t1, $t2
    lui $t1, %hi(battle_drop_label)
    addiu $t1, $t1, %lo(battle_drop_label)
    xori $t2, $s0, BATTLE_DROP_ROW
    movz $a3, $t1, $t2
    lui $t1, %hi(always_great_label)
    addiu $t1, $t1, %lo(always_great_label)
    xori $t2, $s0, ALWAYS_GREAT_ROW
    movz $a3, $t1, $t2
    lui $t1, %hi(inherit_type_label)
    addiu $t1, $t1, %lo(inherit_type_label)
    xori $t2, $s0, INHERIT_TYPE_ROW
    movz $a3, $t1, $t2
    lui $t1, %hi(shrine_label)
    addiu $t1, $t1, %lo(shrine_label)
    xori $t2, $s0, SHRINE_ROW
    movz $a3, $t1, $t2
    lui $t1, %hi(community_label)
    addiu $t1, $t1, %lo(community_label)
    xori $t2, $s0, COMMUNITY_ROW
    movz $a3, $t1, $t2
    move $a0, $s0
    jal .Ldraw_row
    move $a1, $s2
    addiu $s0, $s0, 1
    bne $s0, $s1, .Ldraw_row_loop
    addiu $s2, $s2, 25

    # Scroll track and thumb on the right edge of the 5-row list.
    lw $a0, PALETTE_MUTED_VALUE($s5)
    li $a1, SCROLLBAR_X
    li $a2, 73
    li $a3, 4
    jal .Ldraw_rect_h
    li $t0, 122

    lui $t1, %hi(menu_scroll)
    lw $t1, %lo(menu_scroll)($t1)
    li $t2, 13
    mul $t1, $t1, $t2
    addiu $a2, $t1, 73
    lw $a0, PALETTE_SELECTED_ROW($s5)
    li $a1, SCROLLBAR_X
    li $a3, 4
    jal .Ldraw_rect_h
    li $t0, 70

    # Full-width description panel aligned with the menu rows and a 3px left accent bar.
    lw $a0, PALETTE_SELECTED_ROW($s5)
    li $a1, ROW_X
    li $a2, 217
    li $a3, 3
    jal .Ldraw_rect_h
    li $t0, 22

    lw $a0, PALETTE_MUTED_VALUE($s5)
    li $a1, ROW_X + 3
    li $a2, 217
    li $a3, ROW_WIDTH - 3
    jal .Ldraw_rect_h
    li $t0, 22

    lui $t0, %hi(menu_row)
    lw $t0, %lo(menu_row)($t0)
    lui $a3, %hi(difficulty_description)
    addiu $a3, $a3, %lo(difficulty_description)
    lui $t1, %hi(xp_description)
    addiu $t1, $t1, %lo(xp_description)
    xori $t2, $t0, XP_ROW
    movz $a3, $t1, $t2
    lui $t1, %hi(money_description)
    addiu $t1, $t1, %lo(money_description)
    xori $t2, $t0, MONEY_ROW
    movz $a3, $t1, $t2
    lui $t1, %hi(bench_xp_description)
    addiu $t1, $t1, %lo(bench_xp_description)
    xori $t2, $t0, BENCH_XP_ROW
    movz $a3, $t1, $t2
    lui $t1, %hi(battle_drop_description)
    addiu $t1, $t1, %lo(battle_drop_description)
    xori $t2, $t0, BATTLE_DROP_ROW
    movz $a3, $t1, $t2
    lui $t1, %hi(always_great_description)
    addiu $t1, $t1, %lo(always_great_description)
    xori $t2, $t0, ALWAYS_GREAT_ROW
    movz $a3, $t1, $t2
    lui $t1, %hi(inherit_type_description)
    addiu $t1, $t1, %lo(inherit_type_description)
    xori $t2, $t0, INHERIT_TYPE_ROW
    movz $a3, $t1, $t2
    lui $t1, %hi(shrine_description)
    addiu $t1, $t1, %lo(shrine_description)
    xori $t2, $t0, SHRINE_ROW
    movz $a3, $t1, $t2
    lui $t1, %hi(community_description)
    addiu $t1, $t1, %lo(community_description)
    xori $t0, $t0, COMMUNITY_ROW
    movz $a3, $t1, $t0
    lw $a0, PALETTE_DESCRIPTION($s5)
    li $t2, LABEL_X
    jal .Ldraw_text
    li $t3, 220

    # Draw the native bottom-right PlayStation button guide sprite.
    lw $t0, 20($sp)
    addiu $sp, $sp, -16
    lwc1 $f0, 0x08($t0)
    lwc1 $f1, 0x388($t0)
    add.s $f0, $f0, $f1
    lui $t1, 0x4420             # 640.0f
    mtc1 $t1, $f1
    add.s $f0, $f0, $f1
    swc1 $f0, 16($sp)
    lwc1 $f0, 0x0c($t0)
    lwc1 $f1, 0x38c($t0)
    add.s $f0, $f0, $f1
    lui $t1, 0x43c8             # 400.0f
    mtc1 $t1, $f1
    add.s $f0, $f0, $f1
    swc1 $f0, 20($sp)
    addiu $a0, $sp, 16
    move $a1, $s3
    lhu $a2, 0x34($t0)
    lw $a3, 0x42c($t0)
    jal game_draw_button_guide
    mtc1 $zero, $f12
    addiu $sp, $sp, 16

.Ldraw_return:
    lw $ra, 44($sp)
    lw $s0, 32($sp)
    lw $s1, 36($sp)
    lw $s2, 40($sp)
    lw $s3, 8($sp)
    lw $s4, 12($sp)
    lw $s5, 24($sp)
    jr $ra
    addiu $sp, $sp, 48

.Lconfig_draw_with_hint:
    addiu $sp, $sp, -32
    sw $ra, 28($sp)
    sw $a1, 16($sp)
    sw $s3, 12($sp)
    sw $s4, 8($sp)
    sw $s5, 4($sp)
    jal .Lconfig_draw
    nop
    lw $a1, 16($sp)
    lbu $t1, CONFIG_PALETTE_INDEX($a1)
    xori $t1, $t1, 1
    sltiu $t1, $t1, 1
    sll $t1, $t1, 5
    lui $s5, %hi(menu_palettes)
    addiu $s5, $s5, %lo(menu_palettes)
    addu $s5, $s5, $t1
    li $s3, 255
    move $s4, $zero
    lw $a0, PALETTE_SELECTED_ROW($s5)
    li $a1, ROW_X
    li $a2, 246
    li $a3, 132
    jal .Ldraw_rect_h
    li $t0, 18
    lui $a3, %hi(config_entry_hint)
    addiu $a3, $a3, %lo(config_entry_hint)
    lw $a0, PALETTE_SELECTED_TEXT($s5)
    li $t2, ROW_X + 8
    jal .Ldraw_text
    li $t3, 245
    lw $s5, 4($sp)
    lw $s4, 8($sp)
    lw $s3, 12($sp)
    lw $ra, 28($sp)
    jr $ra
    addiu $sp, $sp, 32

.Lconfig_draw:
    addiu $sp, $sp, -160
    j game_config_draw_continue
    sw $s2, 72($sp)
