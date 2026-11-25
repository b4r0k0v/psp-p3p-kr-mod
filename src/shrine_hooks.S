.set noreorder
.set noat

.text
.global p3p_shrine_selection_hook
p3p_shrine_selection_hook:
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

# sprite_id, x, y, scale (0x1000 = 1.0) using the resident c_main_01.spr sheet.
.global p3p_draw_key_icon
p3p_draw_key_icon:
    addiu $sp, $sp, -48
    sw $ra, 44($sp)
    sw $a0, 24($sp)
    sw $a1, 28($sp)
    sw $a2, 32($sp)
    sw $a3, 36($sp)
    lui $a0, %hi(c_main_01_name)
    jal game_find_sprite
    addiu $a0, $a0, %lo(c_main_01_name)
    beqz $v0, .Lkey_icon_done
    nop
    lw $t4, 28($sp)
    lw $t5, 32($sp)
    mtc1 $t4, $f12
    mtc1 $t5, $f13
    cvt.s.w $f12, $f12
    cvt.s.w $f13, $f13
    mtc1 $zero, $f14
    mtc1 $zero, $f15
    move $a0, $v0
    lw $a1, 24($sp)
    li $a2, 255
    li $a3, 255
    li $t0, 255
    li $t1, 255
    lw $t2, 36($sp)
    move $t3, $t2
    sw $zero, 0($sp)
    jal game_draw_sprite
    sw $zero, 4($sp)
.Lkey_icon_done:
    lw $ra, 44($sp)
    jr $ra
    addiu $sp, $sp, 48

c_main_01_name:
    .asciz "c_main_01.spr"

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
.global shrine_label_decide
shrine_label_decide:
    # 결정
    .byte 0x83, 0xc4, 0x8f, 0xc5, 0
.global shrine_label_back
shrine_label_back:
    # 뒤로
    .byte 0x87, 0x93, 0x88, 0xc3, 0
.global shrine_label_page
shrine_label_page:
    # 페이지
    .byte 0x93, 0xdb, 0x8f, 0x8f, 0x90, 0x97, 0
.global shrine_label_deposit
shrine_label_deposit:
    # 맡기기
    .byte 0x89, 0x96, 0x84, 0xa3, 0x84, 0xa3, 0
.global shrine_label_cancel
shrine_label_cancel:
    # 취소
    .byte 0x91, 0xc8, 0x8c, 0x9d, 0
.global shrine_label_slash
shrine_label_slash:
    .asciz "/"

.balign 4
.global shrine_palettes
shrine_palettes:
    # Background, selected row, heading, controls, description box (RRGGBBAA).
    .word 0x17233ef8, 0x42577fff, 0xfdd37dff, 0xd7def1ff, 0x0d1528f0 # Male route
    .word 0x39192ef8, 0x814364ff, 0xffde66ff, 0xfddae9ff, 0x240e1df0 # Female route

.global p3p_shrine_selection_state
p3p_shrine_selection_state:
    .space 528
