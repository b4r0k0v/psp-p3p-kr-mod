BUILD := build/rewards
FUSION_HOOKS := $(BUILD)/fusion_hooks.elf
OBJECTS := $(addprefix $(BUILD)/,gameplay_hooks.o mod_settings_menu.o fusion_hooks.o fusion_inheritance.o shrine_hooks.o shrine_selection.o battle_drop_hooks.o battle_drop.o bench_xp_hooks.o bench_xp.o)
GAMEPLAY_SYMBOLS := \
	--defsym game_hero_reward_return=0x252c4c \
	--defsym game_persona_max_level=0x252f7c \
	--defsym game_persona_reward_continue=0x252fa0 \
	--defsym game_coin_reward_continue=0x15661c \
	--defsym game_config_input_continue=0x128858 \
	--defsym game_config_input_return=0x12888c \
	--defsym game_config_draw_continue=0x126ed4 \
	--defsym game_community_affinity_compare_continue=0xc1b8 \
	--defsym game_flow_command_return=0x1729b0 \
	--defsym game_flow_get_variable_continue=0x175af0 \
	--defsym game_flow_random_continue=0x1757dc \
	--defsym game_frame_finish_continue=0x310fa4 \
	--defsym game_get_flow_variable=0xa2f8 \
	--defsym game_flag_get=0x8154 \
	--defsym game_item_record=0x95d4 \
	--defsym game_item_count=0x9634 \
	--defsym game_item_set=0x9694 \
	--defsym game_item_name=0xb4cc \
	--defsym game_enemy_table_ptr=0x43dbdc \
	--defsym game_battle_state_ptr=0x43dbf8 \
	--defsym game_battle_drop_continue=0x205c88 \
	--defsym game_battle_drop_return=0x205bdc \
	--defsym game_battle_drop_grant_continue=0x2531c0 \
	--defsym game_battle_result_draw_continue=0x2526c8 \
	--defsym game_battle_grant_continue=0x2530b4 \
	--defsym game_is_member_available=0x112ff0 \
	--defsym game_companion_record=0xd83c \
	--defsym game_persona_level=0xc728 \
	--defsym game_persona_exp=0x9c18 \
	--defsym game_persona_exp_for_level=0xc754 \
	--defsym game_calc_base_xp=0x252af8 \
	--defsym game_persona_calc_level_up=0xeefc \
	--defsym game_persona_add_exp=0xf638 \
	--defsym game_persona_can_level_up=0xf5ec \
	--defsym game_persona_apply_level_up=0xf820 \
	--defsym game_skill_help_handle=0x3bb70c \
	--defsym game_bmd_count=0x1808f4 \
	--defsym game_draw_bmd_text=0x1811b0 \
	--defsym game_is_female_route=0x9718 \
	--defsym game_button_edges=0x693948 \
	--defsym game_time_us=0x37c7e0 \
	--defsym game_prepare_rect=0x10cbf4 \
	--defsym game_draw_rect=0x2a6654 \
	--defsym game_draw_text=0x17b398
LINK_PAYLOAD := psp-ld -EL --emit-relocs --section-start=.text=0x6be000 --entry=p3p_base_xp_hook

.PHONY: all
all: $(FUSION_HOOKS)

$(BUILD):
	mkdir -p $@

$(BUILD)/%.o: src/%.S | $(BUILD)
	psp-gcc -Wa,--fatal-warnings -G0 -mno-abicalls -fno-pic -MMD -MP -c $< -o $@

$(BUILD)/%.o: src/%.c | $(BUILD)
	psp-gcc -Wall -Wextra -Werror -Os -G0 -mno-abicalls -fno-pic -ffreestanding \
		-fno-strict-aliasing -fno-jump-tables -c $< -o $@

$(FUSION_HOOKS): $(OBJECTS)
	$(LINK_PAYLOAD) $(GAMEPLAY_SYMBOLS) \
		--defsym game_fusion_pool_cleanup=0x57714 \
		--defsym game_fusion_pool_continue=0x57610 \
		--defsym game_fusion_input_continue=0x4d0b4 \
		--defsym game_fusion_help_guide_continue=0x542a8 \
		--defsym game_fusion_after_help_guide=0x542bc \
		--defsym game_fusion_open_help=0x13e9b0 \
		--defsym game_fusion_close_help=0x13e0bc \
		--defsym game_fusion_button_edges=0x693948 -o $@ $^

-include $(OBJECTS:.o=.d)
