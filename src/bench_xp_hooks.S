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

.global bench_toast_title
bench_toast_title:
    # 벤치 레벨업
    .byte 0x8a, 0xb4, 0x91, 0xdc, 0x20, 0x88, 0xae, 0x8a, 0xb6, 0x8d, 0xfe, 0
.global bench_name_2
bench_name_2:
    # 유카리
    .byte 0x8e, 0xf2, 0x91, 0xe6, 0x89, 0x81, 0
.global bench_name_3
bench_name_3:
    # 아이기스
    .byte 0x8d, 0xcd, 0x8f, 0x8f, 0x84, 0xa3, 0x8c, 0xe3, 0
.global bench_name_4
bench_name_4:
    # 미츠루
    .byte 0x89, 0xfd, 0x91, 0xd4, 0x88, 0xdc, 0
.global bench_name_5
bench_name_5:
    # 준페이
    .byte 0x8f, 0xf9, 0x93, 0xdb, 0x8f, 0x8f, 0
.global bench_name_7
bench_name_7:
    # 아키히코
    .byte 0x8d, 0xcd, 0x92, 0xc9, 0x95, 0xaa, 0x92, 0x95, 0
.global bench_name_8
bench_name_8:
    # 아마다
    .byte 0x8d, 0xcd, 0x89, 0x89, 0x86, 0xb4, 0
.global bench_name_9
bench_name_9:
    # 아라가키
    .byte 0x8d, 0xcd, 0x88, 0x8a, 0x83, 0x84, 0x92, 0xc9, 0
.global bench_name_10
bench_name_10:
    # 코로마루
    .byte 0x92, 0x95, 0x88, 0xc3, 0x89, 0x89, 0x88, 0xdc, 0

.balign 4
.global p3p_bench_toast_state
.global p3p_bench_toast_count
p3p_bench_toast_state:
p3p_bench_toast_count:
    .space 64
