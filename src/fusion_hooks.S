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
    jal p3p_fusion_draw_help_guide
    nop
    beqz $v0, .Lstock_help_guide
    nop
    lw $ra, 28($sp)
    j game_fusion_after_help_guide
    addiu $sp, $sp, 32

.Lstock_help_guide:
    lw $ra, 28($sp)
    addiu $sp, $sp, 32
    move $a0, $s2
    j game_fusion_help_guide_continue
    li $a1, 16

.global p3p_fusion_inherit_type_hook
p3p_fusion_inherit_type_hook:
    bgtz $v1, .Linherit_type_done
    nop
    lui $v0, %hi(p3p_ignore_inherit_type)
    lw $v0, %lo(p3p_ignore_inherit_type)($v0)
    beqz $v0, .Linherit_type_done
    nop
    li $v1, 100
.Linherit_type_done:
    sll $v0, $v1, 3
    j game_fusion_inherit_type_continue
    sll $v1, $v1, 1

# Draw words separately so the font does not draw a glyph between them.
.global fusion_inherit
fusion_inherit:
    # 계승
    .byte 0x83, 0xcb, 0x8c, 0xeb, 0
.global fusion_prev
fusion_prev:
    # 이전
    .byte 0x8f, 0x8f, 0x8f, 0xbf, 0

.balign 4
.global p3p_fusion_state
p3p_fusion_state:
    .space 2048
