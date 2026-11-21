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
    lui $t3, %hi(menu_row)
    j game_config_input_return
    sw $zero, %lo(menu_row)($t3)

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

    andi $t2, $a0, RIGHT | LEFT
    li $t3, RIGHT
    beq $t2, $t3, .Lchange_mode
    li $t0, 1
    li $t3, LEFT
    bne $t2, $t3, .Lreturn_menu_input
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
    j game_config_input_return
    sw $t3, 0($t2)
.Lreturn_menu_input:
    j game_config_input_return
    nop
.Lclose_menu:
    begin_transition MENU_CLOSING
    j game_config_input_return
    nop
.Lconfig_input:
    j game_config_input_continue
    andi $v1, $a0, 0x2000
