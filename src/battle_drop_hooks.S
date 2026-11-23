.set noreorder
.set noat

.text
.global p3p_battle_drop_hook
p3p_battle_drop_hook:
    lui $t0, %hi(p3p_battle_drop_100)
    lw $t0, %lo(p3p_battle_drop_100)($t0)
    bnez $t0, .Ldrop_100_on
    nop
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

.balign 4
.global p3p_battle_drop_state
p3p_battle_drop_state:
    .space 128
