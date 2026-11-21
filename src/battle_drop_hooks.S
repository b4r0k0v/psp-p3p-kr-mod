.set noreorder
.set noat

.text
.global p3p_battle_drop_hook
p3p_battle_drop_hook:
    lui $t0, %hi(p3p_battle_drop_100)
    lw $t0, %lo(p3p_battle_drop_100)($t0)
    bnez $t0, .Ldrop_100_on
    lui $t0, %hi(game_battle_state_ptr)
    lw $t0, %lo(game_battle_state_ptr)($t0)
    beqz $t0, .Ldrop_off_resume
    lui $t2, %hi(p3p_extra_drop_count)
    lw $t1, 0xc90($t0)
    bnez $t1, .Ldrop_off_resume
    nop
    sw $zero, %lo(p3p_extra_drop_count)($t2)
.Ldrop_off_resume:
    addu $s1, $v0, $zero
    j game_battle_drop_continue
    lw $v1, 0($sp)

.Ldrop_100_on:
    addiu $sp, $sp, -16
    move $a0, $s0
    jal p3p_collect_enemy_drops
    lw $a1, 2648($s2)
    addiu $sp, $sp, 16
    j game_battle_drop_return
    nop

.global p3p_battle_drop_grant_hook
p3p_battle_drop_grant_hook:
    lui $t0, %hi(p3p_extra_drop_count)
    lw $t0, %lo(p3p_extra_drop_count)($t0)
    beqz $t0, .Lgrant_done
    nop
    addiu $sp, $sp, -16
    jal p3p_grant_extra_drops
    lw $a0, 52($s2)
    addiu $sp, $sp, 16
.Lgrant_done:
    lw $ra, 16($sp)
    j game_battle_drop_grant_continue
    lw $s3, 12($sp)

.global p3p_battle_result_draw_hook
p3p_battle_result_draw_hook:
    lui $t0, %hi(p3p_extra_drop_count)
    lw $t0, %lo(p3p_extra_drop_count)($t0)
    beqz $t0, .Ldraw_done
    nop
    addiu $sp, $sp, -16
    jal p3p_draw_extra_drops
    move $a0, $s5
    addiu $sp, $sp, 16
.Ldraw_done:
    lw $ra, 116($sp)
    j game_battle_result_draw_continue
    lw $fp, 112($sp)

.balign 4
.global p3p_battle_drop_state
.global p3p_extra_drop_count
p3p_battle_drop_state:
p3p_extra_drop_count:
    .space 128
