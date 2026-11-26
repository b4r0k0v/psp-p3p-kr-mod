.set noreorder
.set noat

.equ SELECT, 0x0001
.equ UP, 0x0010
.equ DOWN, 0x0040
.equ RIGHT, 0x0020
.equ LEFT, 0x0080
.equ CIRCLE, 0x2000
.equ CROSS, 0x4000
.equ CONFIG_PALETTE_INDEX, 0
.equ CONFIG_VARIANT, 24
.equ MENU_OPENING, 1
.equ MENU_OPEN, 2
.equ MENU_CLOSING, 3
.equ OPEN_TIME_US, 120000
.equ CLOSE_TIME_US, 90000
.equ FIRST_TOGGLE_ROW, 2
.equ BENCH_XP_ROW, 2
.equ BATTLE_DROP_ROW, 3
.equ ALWAYS_GREAT_ROW, 4
.equ SHRINE_ROW, 5
.equ COMMUNITY_ROW, 6
.equ ROW_COUNT, 7
.equ VISIBLE_ROWS, 5

.equ PALETTE_TITLE, 0
.equ PALETTE_SELECTED_TEXT, 4
.equ PALETTE_ROW, 8
.equ PALETTE_DESCRIPTION, 12
.equ PALETTE_SELECTED_ROW, 16
.equ PALETTE_IDLE_ROW, 20
.equ PALETTE_VALUE, 24
.equ PALETTE_MUTED_VALUE, 28

.equ ROW_X, 61
.equ LABEL_X, 71
.equ VALUE_X, 247
.equ VALUE_WIDTH, 68
.equ ROW_WIDTH, VALUE_X + VALUE_WIDTH - ROW_X

# Keep the menu in one assembler unit so branch labels and data stay local.
.text
#include "mod_settings_menu/input.inc"
#include "mod_settings_menu/draw.inc"
#include "mod_settings_menu/rows.inc"
#include "mod_settings_menu/data.inc"
