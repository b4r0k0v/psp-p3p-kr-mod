.set noreorder
.set noat

.equ HERO_RETURN, 0x252c4c
.equ COMPANION_RETURN, 0x252d4c
.equ SPECIAL_COMPANION_RETURN, 0x252e30
.equ XP_CAP, 65535
.equ WALLET_CAP, 9999999

.macro save_scratch
    addiu $sp, $sp, -16
    sw $t0, 0($sp)
    sw $t1, 4($sp)
    sw $t2, 8($sp)
    sw $t3, 12($sp)
.endm

.macro restore_scratch
    lw $t0, 0($sp)
    lw $t1, 4($sp)
    lw $t2, 8($sp)
    lw $t3, 12($sp)
    addiu $sp, $sp, 16
.endm

# t0 is a manual mode (1 to 4), t1 is the bounded nonnegative limit.
# Clamp the input first so all operations fit in a signed 32-bit value.
.macro scale_reward reward
    bgtz \reward, .Lpositive\@
    nop
    b .Lscaled\@
    move \reward, $zero
.Lpositive\@:
    sltu $t2, $t1, \reward
    movn \reward, $t1, $t2
    addiu $t0, $t0, -1
    bnez $t0, .Lpower_of_two\@
    srl $t3, \reward, 1
    b .Lclamp\@
    addu \reward, \reward, $t3
.Lpower_of_two\@:
    sllv \reward, \reward, $t0
.Lclamp\@:
    sltu $t2, $t1, \reward
    movn \reward, $t1, $t2
.Lscaled\@:
.endm

.text
.global p3p_base_xp_hook
p3p_base_xp_hook:
    # Displaced move from the common reward function's epilogue.
    move $v0, $a1
    save_scratch
    lui $t0, %hi(p3p_xp_mode)
    lw $t0, %lo(p3p_xp_mode)($t0)
    beqz $t0, .Lbase_return
    sltiu $t2, $t0, 5
    beqz $t2, .Lbase_return
    nop

    # Only the three character callers receive scaling here. The Persona
    # caller applies its own rules after this return and scales separately.
    lui $t1, %hi(game_hero_reward_return)
    addiu $t1, $t1, %lo(game_hero_reward_return)
    beq $ra, $t1, .Lbase_level_checked
    move $t3, $s0
    addiu $t1, $t1, COMPANION_RETURN - HERO_RETURN
    beq $ra, $t1, .Lbase_level_checked
    move $t3, $s1
    addiu $t1, $t1, SPECIAL_COMPANION_RETURN - COMPANION_RETURN
    bne $ra, $t1, .Lbase_return
    nop
.Lbase_level_checked:
    sltiu $t2, $t3, 99
    beqz $t2, .Lbase_return
    ori $t1, $zero, XP_CAP
    scale_reward $v0
.Lbase_return:
    restore_scratch
    jr $ra
    addiu $sp, $sp, 32

.global p3p_persona_xp_hook
p3p_persona_xp_hook:
    # Repeat the original level check and resume its two original paths.
    slti $v0, $s3, 99
    bnez $v0, .Lpersona_eligible
    nop
    j game_persona_max_level
    nop
.Lpersona_eligible:
    save_scratch
    lui $t0, %hi(p3p_xp_mode)
    lw $t0, %lo(p3p_xp_mode)($t0)
    beqz $t0, .Lpersona_continue
    sltiu $t2, $t0, 5
    beqz $t2, .Lpersona_continue
    ori $t1, $zero, XP_CAP
    scale_reward $s1
.Lpersona_continue:
    restore_scratch
    j game_persona_reward_continue
    nop

.global p3p_coin_hook
p3p_coin_hook:
    save_scratch
    lui $t0, %hi(p3p_money_mode)
    lw $t0, %lo(p3p_money_mode)($t0)
    beqz $t0, .Lcoin_continue
    sltiu $t2, $t0, 5
    beqz $t2, .Lcoin_continue
    nop
    li $t1, WALLET_CAP
    max $t3, $s1, $zero
    sltu $t2, $t1, $t3
    movn $t3, $t1, $t2
    subu $t1, $t1, $t3
    scale_reward $s0
.Lcoin_continue:
    restore_scratch
    addu $v0, $s0, $s1
    j game_coin_reward_continue
    ori $a0, $a0, 0x967f

.global p3p_split_xp_hook
p3p_split_xp_hook:
    lui $t0, %hi(p3p_xp_mode)
    lw $t0, %lo(p3p_xp_mode)($t0)
    beqz $t0, .Lsplit_resume
    sltiu $t2, $t0, 5
    beqz $t2, .Lsplit_resume
    nop
    blez $a1, .Lsplit_resume
    andi $t1, $a0, 0xffff
    addiu $t2, $t1, -2
    sltiu $t2, $t2, 9
    beqz $t2, .Lsplit_resume
    nop
    addiu $sp, $sp, -16
    sw $ra, 12($sp)
    sw $a0, 8($sp)
    sw $a1, 4($sp)
    jal game_companion_record
    move $a0, $t1
    beqz $v0, .Lsplit_pop
    nop
    jal game_persona_level
    move $a0, $v0
    sltiu $t2, $v0, 99
    beqz $t2, .Lsplit_pop
    nop
    lw $a1, 4($sp)
    lui $t0, %hi(p3p_xp_mode)
    lw $t0, %lo(p3p_xp_mode)($t0)
    ori $t1, $zero, XP_CAP
    scale_reward $a1
    sw $a1, 4($sp)
.Lsplit_pop:
    lw $ra, 12($sp)
    lw $a0, 8($sp)
    lw $a1, 4($sp)
    addiu $sp, $sp, 16
.Lsplit_resume:
    addiu $sp, $sp, -160
    j game_split_xp_continue
    sw $s1, 148($sp)

#include "progression_hooks.inc"

.balign 4
.global p3p_reward_settings
p3p_reward_settings:
    .ascii "P3PRWD01"
# Keep the setting words in menu row order.
.global p3p_xp_mode
p3p_xp_mode:
    .word 0
.global p3p_money_mode
p3p_money_mode:
    .word 0
.global p3p_community_affinity_zero
p3p_community_affinity_zero:
    .word 0
.global p3p_shrine_wait_one_day
p3p_shrine_wait_one_day:
    .word 0
.global p3p_battle_drop_100
p3p_battle_drop_100:
    .word 0
.global p3p_bench_xp
p3p_bench_xp:
    .word 0
