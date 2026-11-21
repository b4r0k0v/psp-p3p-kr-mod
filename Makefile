BUILD := build/rewards
FUSION_HOOKS := $(BUILD)/fusion_hooks.elf
OBJECTS := $(addprefix $(BUILD)/,reward_hooks.o reward_menu.o fusion_hooks.o fusion_inheritance.o)
REWARD_SYMBOLS := \
	--defsym game_hero_reward_return=0x252c4c \
	--defsym game_persona_max_level=0x252f7c \
	--defsym game_persona_reward_continue=0x252fa0 \
	--defsym game_coin_reward_continue=0x15661c \
	--defsym game_config_input_continue=0x128858 \
	--defsym game_config_input_return=0x12888c \
	--defsym game_config_draw_continue=0x126ed4 \
	--defsym game_community_affinity_compare_continue=0xc1b8 \
	--defsym game_time_us=0x37c7e0 \
	--defsym game_draw_text=0x17b398
LINK_PAYLOAD := psp-ld -EL --emit-relocs --section-start=.text=0x6be000 --entry=p3p_base_xp_hook

.PHONY: all
all: $(FUSION_HOOKS)

$(BUILD):
	mkdir -p $@

$(BUILD)/%.o: src/%.S | $(BUILD)
	psp-gcc -Wa,--fatal-warnings -G0 -mno-abicalls -fno-pic -c $< -o $@

$(BUILD)/%.o: src/%.c | $(BUILD)
	psp-gcc -Wall -Wextra -Werror -Os -G0 -mno-abicalls -fno-pic -ffreestanding \
		-fno-strict-aliasing -fno-jump-tables -c $< -o $@

$(FUSION_HOOKS): $(OBJECTS)
	$(LINK_PAYLOAD) $(REWARD_SYMBOLS) \
		--defsym game_fusion_pool_cleanup=0x57714 \
		--defsym game_fusion_pool_continue=0x57610 \
		--defsym game_fusion_input_continue=0x4d0b4 \
		--defsym game_fusion_help_guide_continue=0x542a8 \
		--defsym game_fusion_after_help_guide=0x542bc \
		--defsym game_fusion_open_help=0x13e9b0 \
		--defsym game_fusion_close_help=0x13e0bc \
		--defsym game_fusion_button_edges=0x693948 -o $@ $^
