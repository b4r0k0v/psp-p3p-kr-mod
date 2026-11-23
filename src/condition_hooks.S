.set noreorder
.set noat

.equ CONDITION_GREAT, 2

.text
.global p3p_set_condition_hook
p3p_set_condition_hook:
    lui $t0, %hi(p3p_always_great)
    lw $t0, %lo(p3p_always_great)($t0)
    beqz $t0, .Lset_cond_resume
    andi $t1, $a1, 0xffff
    li $t2, CONDITION_GREAT
    beq $t1, $t2, .Lset_cond_resume
    nop
    addiu $sp, $sp, -16
    sw $ra, 12($sp)
    sw $a0, 8($sp)
    sw $a1, 4($sp)
    jal p3p_is_condition_target
    seh $a0, $a0
    lw $ra, 12($sp)
    lw $a0, 8($sp)
    lw $a1, 4($sp)
    addiu $sp, $sp, 16
    beqz $v0, .Lset_cond_resume
    nop
    li $a1, CONDITION_GREAT
.Lset_cond_resume:
    addiu $sp, $sp, -16
    j game_set_condition_continue
    sw $s1, 4($sp)

.global p3p_add_stamina_hook
p3p_add_stamina_hook:
    lui $t0, %hi(p3p_always_great)
    lw $t0, %lo(p3p_always_great)($t0)
    beqz $t0, .Ladd_stam_resume
    seh $t1, $a1
    bgez $t1, .Ladd_stam_resume
    nop
    addiu $sp, $sp, -16
    sw $ra, 12($sp)
    sw $a0, 8($sp)
    sw $a1, 4($sp)
    jal p3p_is_condition_target
    seh $a0, $a0
    lw $ra, 12($sp)
    lw $a0, 8($sp)
    lw $a1, 4($sp)
    addiu $sp, $sp, 16
    beqz $v0, .Ladd_stam_resume
    nop
    jr $ra
    move $v0, $zero
.Ladd_stam_resume:
    addiu $sp, $sp, -16
    j game_add_stamina_continue
    sw $s0, 0($sp)

.global p3p_day_condition_hook
p3p_day_condition_hook:
    lui $t0, %hi(p3p_always_great)
    lw $t0, %lo(p3p_always_great)($t0)
    beqz $t0, .Lday_cond_resume
    nop
    j p3p_day_condition_override
    nop
.Lday_cond_resume:
    addiu $sp, $sp, -48
    j game_day_condition_continue
    sw $ra, 32($sp)
