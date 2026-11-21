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
.equ PALETTE_SELECTED_TEXT, 4
.equ PALETTE_ROW, 8
.equ PALETTE_DESCRIPTION, 12
.equ PALETTE_SELECTED_ROW, 16
.equ PALETTE_IDLE_ROW, 20
.equ PALETTE_VALUE, 24
.equ PALETTE_MUTED_VALUE, 28

.equ ROW_X, 61
.equ LABEL_X, 71
.equ VALUE_X, 247
.equ VALUE_WIDTH, 68
.equ ROW_WIDTH, VALUE_X + VALUE_WIDTH - ROW_X

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
    li $t3, 66
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
    lui $a3, %hi(xp_description)
    addiu $a3, $a3, %lo(xp_description)
    lui $t1, %hi(money_description)
    addiu $t1, $t1, %lo(money_description)
    movn $a3, $t1, $t0
    lui $t1, %hi(community_description)
    addiu $t1, $t1, %lo(community_description)
    xori $t0, $t0, COMMUNITY_ROW
    movz $a3, $t1, $t0
    lw $a0, PALETTE_DESCRIPTION($s5)
    li $t2, LABEL_X
    jal .Ldraw_text
    li $t3, 194

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
    li $t2, COMMUNITY_ROW
    beq $t0, $t2, .Ldraw_toggle
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

.balign 4
# Native font colors are packed as RRGGBBAA.
menu_palettes:
    # Male route, blue with gold value highlight.
    .word 0x003c5fff, 0xffffffff, 0x003c5fff, 0x003c5fff
    .word 0x003c5fff, 0x6db3ffff, 0xffc449ff, 0xace3fbff
    # Female route, burgundy with pink value highlight.
    .word 0x6b010aff, 0xffffffff, 0x6b010aff, 0x6b010aff
    .word 0x6b010aff, 0xfddae9ff, 0xffc1dcff, 0xffadc3ff
menu_state:
    .word 0
menu_started_us:
    .word 0
menu_row:
    .word 0
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
community_description:
    # 랭크업 판정에만 적용됩니다.
    .byte 0x88, 0x9e, 0x92, 0xc2, 0x8d, 0xfe, 0x20, 0x93, 0xbe, 0x8f, 0xc5, 0x8e, 0x86, 0x89
    .byte 0x8b, 0x20, 0x8f, 0xbe, 0x8e, 0xd0, 0x87, 0x84, 0x86, 0xaa, 0x86, 0xb4, 0x2e, 0x00
.balign 4
toggle_labels:
    .asciz "ON"
    .byte 0
    .asciz "OFF"
mode_labels:
    .asciz "AUTO"
    .space 3
    .byte 0x82, 0xae, '1', '.', '5', 0, 0, 0 # ×1.5
    .byte 0x82, 0xae, '2', 0, 0, 0, 0, 0     # ×2
    .byte 0x82, 0xae, '4', 0, 0, 0, 0, 0     # ×4
    .byte 0x82, 0xae, '8', 0, 0, 0, 0, 0     # ×8
