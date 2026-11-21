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
