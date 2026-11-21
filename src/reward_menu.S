.set noreorder
.set noat

.equ SELECT, 0x0001
.equ UP, 0x0010
.equ DOWN, 0x0040
.equ RIGHT, 0x0020
.equ LEFT, 0x0080
.equ CROSS, 0x4000
.equ CONFIG_PALETTE_INDEX, 0
.equ CONFIG_VARIANT, 24
.equ MENU_OPENING, 1
.equ MENU_OPEN, 2
.equ MENU_CLOSING, 3
.equ OPEN_TIME_US, 120000
.equ CLOSE_TIME_US, 90000
.equ COMMUNITY_ROW, 2
.equ ROW_COUNT, 3

.equ PALETTE_TITLE, 0
.equ PALETTE_SELECTED, 4
.equ PALETTE_ROW, 8
.equ PALETTE_DESCRIPTION, 12

.macro begin_transition phase
    jal game_time_us
    nop
    lui $t0, %hi(menu_started_us)
    sw $v0, %lo(menu_started_us)($t0)
    lui $t0, %hi(menu_state)
    li $t1, \phase
    sw $t1, %lo(menu_state)($t0)
.endm

.text
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
    li $t4, COMMUNITY_ROW
    beq $t1, $t4, .Ltoggle_community
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
.Ltoggle_community:
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
    sll $t1, $t1, 4             # Each palette contains four 32-bit colors.
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
    li $t2, 120
    li $t3, 76
    jal .Ldraw_text
    nop

    lui $a3, %hi(xp_label)
    addiu $a3, $a3, %lo(xp_label)
    move $a0, $zero
    jal .Ldraw_row
    li $a1, 108

    lui $a3, %hi(money_label)
    addiu $a3, $a3, %lo(money_label)
    li $a0, 1
    jal .Ldraw_row
    li $a1, 137

    lui $a3, %hi(community_label)
    addiu $a3, $a3, %lo(community_label)
    li $a0, COMMUNITY_ROW
    jal .Ldraw_row
    li $a1, 166

    lui $t0, %hi(menu_row)
    lw $t0, %lo(menu_row)($t0)
    li $t1, COMMUNITY_ROW
    beq $t0, $t1, .Ldraw_return
    nop
    lui $a3, %hi(xp_description)
    addiu $a3, $a3, %lo(xp_description)
    lui $t1, %hi(money_description)
    addiu $t1, $t1, %lo(money_description)
    movn $a3, $t1, $t0
    lw $a0, PALETTE_DESCRIPTION($s5)
    li $t2, 120
    jal .Ldraw_text
    li $t3, 203

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

# a0 selects the row, a1 is y, a3 points to the label.
.Ldraw_row:
    addiu $sp, $sp, -32
    sw $ra, 28($sp)
    sw $a0, 20($sp)
    sw $a1, 24($sp)
    lui $t0, %hi(menu_row)
    lw $t0, %lo(menu_row)($t0)
    lw $t2, PALETTE_ROW($s5)
    bne $a0, $t0, .Lrow_color
    nop
    sw $a3, 12($sp)
    lui $a3, %hi(selection_marker)
    addiu $a3, $a3, %lo(selection_marker)
    lw $a0, PALETTE_SELECTED($s5)
    move $t3, $a1
    jal .Ldraw_text
    li $t2, 104
    lw $a3, 12($sp)
    lw $a1, 24($sp)
    lw $t2, PALETTE_SELECTED($s5)
.Lrow_color:
    sw $t2, 16($sp)
    move $a0, $t2
    move $t3, $a1
    jal .Ldraw_text
    li $t2, 120

    lw $t0, 20($sp)
    lui $t1, %hi(p3p_xp_mode)
    addiu $t1, $t1, %lo(p3p_xp_mode)
    sll $t2, $t0, 2
    addu $t1, $t1, $t2
    lw $t1, 0($t1)
    li $t2, COMMUNITY_ROW
    beq $t0, $t2, .Ldraw_toggle
    nop
    sltiu $t0, $t1, 5
    movz $t1, $zero, $t0
    sll $t1, $t1, 3
    lui $a3, %hi(mode_labels)
    addiu $a3, $a3, %lo(mode_labels)
    addu $a3, $a3, $t1
    b .Ldraw_value
    li $t2, 275
.Ldraw_toggle:
    sltu $t1, $zero, $t1
    sll $t1, $t1, 2
    lui $a3, %hi(toggle_labels)
    addiu $a3, $a3, %lo(toggle_labels)
    addu $a3, $a3, $t1
    li $t2, 312
.Ldraw_value:
    lw $a0, 16($sp)
    lw $t3, 24($sp)
    lw $ra, 28($sp)
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

.balign 4
# Native font colors are packed as RRGGBBAA.
menu_palettes:
    .word 0x063754ff, 0x002840ff, 0x254d72ff, 0x154161ff # Male route
    .word 0x65142dff, 0x6b010aff, 0x75364bff, 0x6d2940ff # Female route
menu_state:
    .word 0
menu_started_us:
    .word 0
menu_row:
    .word 0
selection_marker:
    .asciz ">"
# UCKS45140 font encoding. The comments show the decoded menu strings.
menu_title:
    # 모드 설정
    .byte 0x89, 0xc3, 0x87, 0x9e, 0x20, 0x8b, 0xfe, 0x8f, 0xc5, 0
xp_label:
    # 경험치
    .byte 0x83, 0xc9, 0x94, 0xbd, 0x91, 0xdc, 0
money_label:
    # 돈
    .byte 0x86, 0xf0, 0
community_label:
    # 커뮤 랭크업 필요 호감도 0
    .byte 0x91, 0xfa, 0x89, 0xf3, 0x20, 0x88, 0x9e, 0x92, 0xc2, 0x8d, 0xfe, 0x20
    .byte 0x94, 0x9f, 0x8e, 0xc9, 0x20, 0x94, 0xd6, 0x83, 0x8b, 0x86, 0xee, 0x20, '0', 0
xp_description:
    # 전투에서 얻는 경험치에 적용됩니다.
    .byte 0x8f, 0xbf, 0x93, 0x8e, 0x8e, 0x86, 0x8b, 0xf8, 0x20, 0x8d, 0xf9, 0x86, 0x9d, 0x20
    .byte 0x83, 0xc9, 0x94, 0xbd, 0x91, 0xdc, 0x8e, 0x86, 0x20
    .byte 0x8f, 0xbe, 0x8e, 0xd0, 0x87, 0x84, 0x86, 0xaa, 0x86, 0xb4, 0x2e, 0
money_description:
    # 셔플 타임 코인으로 얻는 돈에 적용됩니다.
    .byte 0x8c, 0x90, 0x94, 0x98, 0x20, 0x92, 0xd1, 0x8f, 0x96, 0x20, 0x92, 0x95, 0x8f, 0x91
    .byte 0x8e, 0xfb, 0x88, 0xc3, 0x20, 0x8d, 0xf9, 0x86, 0x9d, 0x20, 0x86, 0xf0, 0x8e, 0x86, 0x20
    .byte 0x8f, 0xbe, 0x8e, 0xd0, 0x87, 0x84, 0x86, 0xaa, 0x86, 0xb4, 0x2e, 0
.balign 4
toggle_labels:
    .asciz "Off"
    .asciz "On"
    .byte 0
mode_labels:
    .asciz "AUTO"
    .space 3
    .byte 0x82, 0xae, '1', '.', '5', 0, 0, 0 # ×1.5
    .byte 0x82, 0xae, '2', 0, 0, 0, 0, 0     # ×2
    .byte 0x82, 0xae, '4', 0, 0, 0, 0, 0     # ×4
    .byte 0x82, 0xae, '8', 0, 0, 0, 0, 0     # ×8
