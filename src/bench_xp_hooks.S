.set noreorder
.set noat

.text
.global p3p_bench_xp_hook
p3p_bench_xp_hook:
    lui $t0, %hi(p3p_bench_xp)
    lw $t0, %lo(p3p_bench_xp)($t0)
    beqz $t0, .Lbench_resume
    nop
    addiu $sp, $sp, -32
    sw $ra, 28($sp)
    sw $a0, 24($sp)
    sw $a1, 20($sp)
    jal p3p_grant_bench_xp
    nop
    lw $ra, 28($sp)
    lw $a0, 24($sp)
    lw $a1, 20($sp)
    addiu $sp, $sp, 32
.Lbench_resume:
    addiu $sp, $sp, -32
    j game_battle_grant_continue
    sw $s0, 0($sp)
