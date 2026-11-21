.set noreorder
.set noat

.text
.global p3p_fusion_pool_hook
p3p_fusion_pool_hook:
    addiu $sp, $sp, -32
    sw $ra, 28($sp)
    sw $v0, 24($sp)
    move $a0, $s6
    move $a1, $s8
    jal p3p_fusion_record_pool
    move $a2, $s7
    lw $ra, 28($sp)
    lw $v0, 24($sp)
    addiu $sp, $sp, 32
    bnez $s7, 1f
    move $s4, $v0
    j game_fusion_pool_cleanup
    nop
1:
    j game_fusion_pool_continue
    nop

.global p3p_fusion_input_hook
p3p_fusion_input_hook:
    addiu $sp, $sp, -32
    sw $ra, 28($sp)
    jal p3p_fusion_preview_input
    sw $a0, 24($sp)
    lw $a0, 24($sp)
    lw $ra, 28($sp)
    beqz $v0, 1f
    addiu $sp, $sp, 32
    jr $ra
    nop
1:
    addiu $sp, $sp, -64
    j game_fusion_input_continue
    sw $s1, 52($sp)

.global p3p_fusion_help_guide_hook
p3p_fusion_help_guide_hook:
    # The original branch has set the footer's coordinates, and a0 still holds its context.
    addiu $sp, $sp, -32
    sw $ra, 28($sp)
    jal p3p_fusion_is_selecting
    nop
    beqz $v0, .Lstock_help_guide
    nop
    lui $a3, %hi(fusion_direction)
    addiu $a3, $a3, %lo(fusion_direction)
    jal p3p_fusion_draw_outlined_caption
    li $t2, 21
    lui $a3, %hi(fusion_choose)
    addiu $a3, $a3, %lo(fusion_choose)
    jal p3p_fusion_draw_outlined_caption
    li $t2, 73
    lw $ra, 28($sp)
    j game_fusion_after_help_guide
    addiu $sp, $sp, 32

.Lstock_help_guide:
    lw $ra, 28($sp)
    addiu $sp, $sp, 32
    move $a0, $s2
    j game_fusion_help_guide_continue
    li $a1, 16

.macro draw_caption_outline dx, y
    lw $a3, 20($sp)
    lw $t2, 24($sp)
    li $a0, 0xf7f5efff
    addiu $t2, $t2, \dx
    jal .Ldraw_caption_layer
    li $t3, \y
.endm

.global p3p_fusion_draw_outlined_caption
p3p_fusion_draw_outlined_caption:
    addiu $sp, $sp, -32
    sw $ra, 28($sp)
    sw $t2, 24($sp)
    sw $a3, 20($sp)
    draw_caption_outline -1, 254
    draw_caption_outline 1, 254
    draw_caption_outline 0, 253
    draw_caption_outline 0, 255
    lw $a3, 20($sp)
    lw $t2, 24($sp)
    li $a0, 0x1c1c1cff
    lw $ra, 28($sp)
    li $t3, 254
    addiu $sp, $sp, 32
.Ldraw_caption_layer:
    move $a1, $zero
    li $a2, 1
    mtc1 $t2, $f12
    mtc1 $t3, $f13
    cvt.s.w $f12, $f12
    cvt.s.w $f13, $f13
    mtc1 $zero, $f14
    move $t0, $zero
    j game_draw_text
    move $t1, $zero

# Draw words separately so the font does not draw a glyph between them.
fusion_direction:
    # 방향키로
    .byte 0x8a, 0x97, 0x94, 0xb7, 0x92, 0xc9, 0x88, 0xc3, 0
fusion_choose:
    # 선택
    .byte 0x8b, 0xfc, 0x92, 0xdc, 0

.balign 4
.global p3p_fusion_state
p3p_fusion_state:
    .space 2048
