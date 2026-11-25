.macro begin_transition phase
    jal game_time_us
    nop
    lui $t0, %hi(menu_started_us)
    sw $v0, %lo(menu_started_us)($t0)
    lui $t0, %hi(menu_state)
    li $t1, \phase
    sw $t1, %lo(menu_state)($t0)
.endm

.global p3p_config_input_hook
p3p_config_input_hook:
    lw $a0, 4($v0)
    lw $t0, CONFIG_VARIANT($s1)
    bnez $t0, .Lconfig_input
    lui $t0, %hi(menu_state)
    lw $t1, %lo(menu_state)($t0)
    bnez $t1, .Lmenu_input
    andi $t2, $a0, SELECT
    beqz $t2, .Lconfig_input
    nop
    begin_transition MENU_OPENING
    lui $t3, %hi(menu_scroll)
    sw $zero, %lo(menu_scroll)($t3)
    lui $t3, %hi(menu_row)
    sw $zero, %lo(menu_row)($t3)
    move $a0, $zero
    li $a1, 1
    move $a2, $zero
    b .Lplay_se_and_return
    li $a3, 3

.Lmenu_input:
    li $t3, MENU_OPEN
    bne $t1, $t3, .Lreturn_menu_input
    nop
    andi $t2, $a0, CROSS
    bnez $t2, .Lclose_menu
    andi $t2, $a0, UP | DOWN
    lui $t3, %hi(menu_row)
    lw $t1, %lo(menu_row)($t3)
    xori $t0, $t2, DOWN
    sltiu $t0, $t0, 1
    xori $t2, $t2, UP
    sltiu $t2, $t2, 1
    xor $t5, $t0, $t2
    addu $t1, $t1, $t0
    subu $t1, $t1, $t2
    bgez $t1, .Lwrap_row_down
    nop
    li $t1, ROW_COUNT - 1
.Lwrap_row_down:
    li $t0, ROW_COUNT
    xor $t0, $t1, $t0
    movz $t1, $zero, $t0
    sw $t1, %lo(menu_row)($t3)
    lui $t4, %hi(menu_scroll)
    lw $t2, %lo(menu_scroll)($t4)
    sltu $t0, $t1, $t2
    bnez $t0, .Lset_scroll_top
    addiu $t0, $t2, VISIBLE_ROWS
    sltu $t0, $t1, $t0
    bnez $t0, .Lscroll_done
    nop
    addiu $t2, $t1, -(VISIBLE_ROWS - 1)
    b .Lstore_scroll
    nop
.Lset_scroll_top:
    move $t2, $t1
.Lstore_scroll:
    sw $t2, %lo(menu_scroll)($t4)
.Lscroll_done:

    andi $t2, $a0, RIGHT | LEFT
    li $t3, RIGHT
    beq $t2, $t3, .Lchange_mode
    li $t0, 1
    li $t3, LEFT
    bne $t2, $t3, .Lmaybe_cursor_se
    li $t0, -1
.Lchange_mode:
    lui $t2, %hi(p3p_xp_mode)
    addiu $t2, $t2, %lo(p3p_xp_mode)
    sll $t3, $t1, 2
    addu $t2, $t2, $t3
    lw $t3, 0($t2)
    sltiu $t4, $t1, COMMUNITY_ROW
    beqz $t4, .Ltoggle_setting
    nop
    addu $t3, $t3, $t0
    li $t0, 5
    bne $t3, $t0, .Lwrap_left
    nop
    move $t3, $zero
.Lwrap_left:
    bgez $t3, .Lstore_mode
    nop
    li $t3, 4
    b .Lstore_mode
    nop
.Ltoggle_setting:
    xori $t3, $t3, 1
.Lstore_mode:
    sw $t3, 0($t2)
    li $t0, ALWAYS_GREAT_ROW
    bne $t1, $t0, .Lplay_change_se
    nop
    beqz $t3, .Lplay_change_se
    nop
    jal p3p_apply_always_great
    nop
.Lplay_change_se:
    move $a0, $zero
    li $a1, 1
    move $a2, $zero
    b .Lplay_se_and_return
    li $a3, 5
.Lmaybe_cursor_se:
    beqz $t5, .Lreturn_menu_input
    nop
    move $a0, $zero
    li $a1, 1
    move $a2, $zero
    move $a3, $zero
.Lplay_se_and_return:
    jal game_play_se
    nop
.Lreturn_menu_input:
    j game_config_input_return
    nop
.Lclose_menu:
    begin_transition MENU_CLOSING
    move $a0, $zero
    move $a1, $zero
    move $a2, $zero
    b .Lplay_se_and_return
    li $a3, 2
.Lconfig_input:
    j game_config_input_continue
    andi $v1, $a0, 0x2000
