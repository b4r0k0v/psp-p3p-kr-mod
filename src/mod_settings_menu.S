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
.equ DIFFICULTY_ROW, 0
.equ XP_ROW, 1
.equ MONEY_ROW, 2
.equ FIRST_TOGGLE_ROW, 3
.equ BENCH_XP_ROW, 3
.equ BATTLE_DROP_ROW, 4
.equ ALWAYS_GREAT_ROW, 5
.equ INHERIT_TYPE_ROW, 6
.equ SHRINE_ROW, 7
.equ COMMUNITY_ROW, 8
.equ ROW_COUNT, 9
.equ VISIBLE_ROWS, 5

.equ FLAG_DIFF_BEGINNER, 4891
.equ FLAG_DIFF_EASY, 4881
.equ FLAG_DIFF_HARD, 4889
.equ FLAG_DIFF_MANIACS, 4890

.equ PALETTE_TITLE, 0
.equ PALETTE_SELECTED_TEXT, 4
.equ PALETTE_ROW, 8
.equ PALETTE_DESCRIPTION, 12
.equ PALETTE_SELECTED_ROW, 16
.equ PALETTE_IDLE_ROW, 20
.equ PALETTE_VALUE, 24
.equ PALETTE_MUTED_VALUE, 28

.equ ROW_X, 44
.equ LABEL_X, 54
.equ VALUE_X, 234
.equ VALUE_WIDTH, 104
.equ ROW_WIDTH, VALUE_X + VALUE_WIDTH - ROW_X
.equ SCROLLBAR_X, VALUE_X + VALUE_WIDTH + 6

# Keep the menu in one assembler unit so branch labels and data stay local.
.text
#include "mod_settings_menu/input.inc"
#include "mod_settings_menu/draw.inc"
#include "mod_settings_menu/rows.inc"
#include "mod_settings_menu/data.inc"
