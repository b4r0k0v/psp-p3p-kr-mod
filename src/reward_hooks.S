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

.global p3p_community_affinity_hook
p3p_community_affinity_hook:
    lui $v1, %hi(p3p_community_affinity_zero)
    lw $v1, %lo(p3p_community_affinity_zero)($v1)
    bnez $v1, .Lzero_affinity_threshold
    nop
    lhu $v1, 16($v0)
    j game_community_affinity_compare_continue
    sltu $v1, $s1, $v1
.Lzero_affinity_threshold:
    j game_community_affinity_compare_continue
    move $v1, $zero            # Unsigned accumulated affinity is always >= 0.

# COMM 0x002e reads a script variable. In h12_01.bf only the three reads
# of variable 94 in HIT_SCRIPT_03 control whether the card is collectible.
.global p3p_shrine_wait_hook
p3p_shrine_wait_hook:
    lui $t0, %hi(p3p_shrine_wait_zero)
    lw $t0, %lo(p3p_shrine_wait_zero)($t0)
    beqz $t0, .Lget_flow_variable
    lui $t0, %hi(game_flow_command_return)
    addiu $t0, $t0, %lo(game_flow_command_return)
    bne $ra, $t0, .Lget_flow_variable
    li $t1, 355
    # The dispatcher preserves the current script in s1 and its code index in s3.
    beq $s3, $t1, .Lcheck_shrine_script
    li $t1, 360
    beq $s3, $t1, .Lcheck_shrine_script
    li $t1, 372
    bne $s3, $t1, .Lget_flow_variable
    nop
.Lcheck_shrine_script:
    lw $t0, 0x124($s1)          # Bytecode base.
    # Match the procedure start and its first wait-counter read.
    # All signature words are at or before the current instruction.
    lw $t1, 163*4($t0)
    li $t2, 0x00040007
    bne $t1, $t2, .Lget_flow_variable
    lw $t1, 351*4($t0)
    li $t2, 0x0009001d
    bne $t1, $t2, .Lget_flow_variable
    lw $t1, 352*4($t0)
    li $t2, 0x00000008
    bne $t1, $t2, .Lget_flow_variable
    lw $t1, 355*4($t0)
    li $t2, 0x002e0008
    bne $t1, $t2, .Lget_flow_variable
    sll $t1, $s3, 2
    addu $t0, $t0, $t1
    lw $t1, -4($t0)
    li $t2, 0x005e001d         # PUSHIS 94 immediately before COMM.
    bne $t1, $t2, .Lget_flow_variable
    nop

    addiu $sp, $sp, -16
    # JAL updates ra before the delay slot, so save the caller's link first.
    sw $ra, 12($sp)
    jal .Lget_flow_variable
    nop
    lw $t0, 0x110($s1)          # Integer result set by the original command.
    addiu $t1, $t0, -1
    sltiu $t1, $t1, 5
    beqz $t1, .Lreturn_shrine_wait
    li $t0, 6                  # Preserve 0 (no order) and already ready orders.
    sw $t0, 0x110($s1)
.Lreturn_shrine_wait:
    lw $ra, 12($sp)
    jr $ra
    addiu $sp, $sp, 16

.Lget_flow_variable:
    addiu $sp, $sp, -16
    j game_flow_get_variable_continue
    sw $ra, 0($sp)

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
.global p3p_shrine_wait_zero
p3p_shrine_wait_zero:
    .word 0
