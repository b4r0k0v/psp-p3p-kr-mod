.set noreorder
.set noat

.text
.global p3p_shrine_selection_hook
p3p_shrine_selection_hook:
    lui $t0, %hi(p3p_shrine_card_selection)
    lw $t0, %lo(p3p_shrine_card_selection)($t0)
    beqz $t0, .Lrandom
    lui $t0, %hi(game_flow_command_return)
    addiu $t0, $t0, %lo(game_flow_command_return)
    bne $ra, $t0, .Lrandom
    li $t0, 445
    bne $s3, $t0, .Lrandom
    nop

    # The index alone can occur in an unrelated FlowScript.
    lw $t0, 0x124($s1)
    lw $t1, 163*4($t0)
    li $t2, 0x00040007
    bne $t1, $t2, .Lrandom
    lw $t1, 355*4($t0)
    li $t2, 0x002e0008
    bne $t1, $t2, .Lrandom
    lw $t1, 423*4($t0)
    li $t2, 0x50040008
    bne $t1, $t2, .Lrandom
    lw $t1, 444*4($t0)
    li $t2, 0x00ff001d
    bne $t1, $t2, .Lrandom
    nop

    j p3p_shrine_choose_card
    move $a0, $s1
.Lrandom:
    addiu $sp, $sp, -16
    j game_flow_random_continue
    sw $ra, 4($sp)

.global p3p_shrine_draw_hook
p3p_shrine_draw_hook:
    # Run while the current frame's display list is open.
    sw $ra, 20($sp)
    sw $s4, 16($sp)
    lui $t0, %hi(p3p_shrine_selection_state)
    lw $t0, %lo(p3p_shrine_selection_state)($t0)
    beqz $t0, .Lframe_finish
    nop
    addiu $sp, $sp, -16
    sw $v0, 0($sp)
    jal p3p_shrine_draw_selection
    sw $a0, 4($sp)
    lw $v0, 0($sp)
    lw $a0, 4($sp)
    addiu $sp, $sp, 16
.Lframe_finish:
    lui $t0, %hi(p3p_bench_toast_count)
    lw $t0, %lo(p3p_bench_toast_count)($t0)
    beqz $t0, .Lframe_done
    nop
    addiu $sp, $sp, -16
    sw $v0, 0($sp)
    jal p3p_draw_bench_toast
    sw $a0, 4($sp)
    lw $v0, 0($sp)
    lw $a0, 4($sp)
    addiu $sp, $sp, 16
.Lframe_done:
    j game_frame_finish_continue
    nop

# RRGGBBAA, x, y, string. The original font API takes float coordinates.
.global p3p_shrine_text
p3p_shrine_text:
    mtc1 $a1, $f12
    mtc1 $a2, $f13
    cvt.s.w $f12, $f12
    cvt.s.w $f13, $f13
    mtc1 $zero, $f14
    move $a1, $zero
    li $a2, 1
    move $t0, $zero
    j game_draw_text
    move $t1, $zero

# RRGGBBAA, x, y, bmd_handle, message_index.
.global p3p_shrine_bmd_text
p3p_shrine_bmd_text:
    mtc1 $a1, $f12
    mtc1 $a2, $f13
    cvt.s.w $f12, $f12
    cvt.s.w $f13, $f13
    mtc1 $zero, $f14
    move $t1, $t0
    move $t0, $a3
    li $a1, 1
    move $a2, $zero
    j game_draw_bmd_text
    li $a3, 1

# RRGGBBAA, x, y, width, height. Match the rectangle API used by mod settings.
.global p3p_shrine_rect
p3p_shrine_rect:
    addiu $sp, $sp, -32
    sw $ra, 28($sp)
    wsbw $a0, $a0
    sw $a0, 0($sp)
    mtc1 $a1, $f0
    mtc1 $a2, $f1
    mtc1 $a3, $f2
    mtc1 $t0, $f3
    cvt.s.w $f0, $f0
    cvt.s.w $f1, $f1
    cvt.s.w $f2, $f2
    cvt.s.w $f3, $f3
    swc1 $f0, 4($sp)
    swc1 $f1, 8($sp)
    swc1 $f2, 12($sp)
    jal game_prepare_rect
    swc1 $f3, 16($sp)
    move $a0, $sp
    addiu $a1, $sp, 4
    move $a2, $zero
    jal game_draw_rect
    mtc1 $zero, $f12
    lw $ra, 28($sp)
    jr $ra
    addiu $sp, $sp, 32

.global shrine_list_title
shrine_list_title:
    # 복제할 카드 선택
    .byte 0x8a, 0xc8, 0x8f, 0xc7, 0x94, 0xa7, 0x20, 0x91, 0xe6, 0x87, 0x9e
    .byte 0x20, 0x8b, 0xfc, 0x92, 0xdc, 0
.global shrine_quantity
shrine_quantity:
    # 보유
    .byte 0x8a, 0xc7, 0x8e, 0xf2, 0
.global shrine_confirm_title
shrine_confirm_title:
    # 카드를 맡기겠습니까?
    .byte 0x91, 0xe6, 0x87, 0x9e, 0x88, 0xf9, 0x20, 0x89, 0x96, 0x84, 0xa3
    .byte 0x83, 0xbd, 0x8c, 0xe9, 0x86, 0xaa, 0x84, 0xaf, '?', 0
.global shrine_controls
shrine_controls:
    # ○ 결정 / × 뒤로
    .byte 0x82, 0xc2, 0x20, 0x83, 0xc4, 0x8f, 0xc5, 0x20, '/', 0x20
    .byte 0x82, 0xae, 0x20, 0x87, 0x93, 0x88, 0xc3, 0

.balign 4
.global shrine_palettes
shrine_palettes:
    # Background, selected row, heading, controls (RRGGBBAA).
    .word 0x17233ef8, 0x42577fff, 0xfdd37dff, 0xd7def1ff # Male route
    .word 0x39192ef8, 0x814364ff, 0xffc1dcff, 0xfddae9ff # Female route

.global p3p_shrine_selection_state
p3p_shrine_selection_state:
    .space 528
