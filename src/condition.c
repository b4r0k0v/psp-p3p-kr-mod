#include <stdint.h>

enum {
    CONDITION_GREAT = 2u,
    HERO_ID = 1,
    MIN_COMPANION_ID = 2,
    MAX_COMPANION_ID = 10,
};

#ifndef P3P_CONDITION_HOST_TEST
extern volatile uint32_t p3p_always_great;
extern int game_is_member_available(int char_id);
extern void game_set_condition(int16_t char_id, uint16_t cond);
extern uint16_t game_get_max_stamina(int16_t char_id);
extern void game_set_stamina(int16_t char_id, int16_t val);
extern void game_flag_set(int flag_id, int value);
#endif

int p3p_is_condition_target(int16_t char_id) {
    if (char_id == HERO_ID) {
        return 1;
    }
    if (char_id >= MIN_COMPANION_ID && char_id <= MAX_COMPANION_ID) {
        return game_is_member_available(char_id) != 0;
    }
    return 0;
}

void p3p_apply_always_great(void) {
    for (int16_t cid = HERO_ID; cid <= MAX_COMPANION_ID; ++cid) {
        if (!p3p_is_condition_target(cid)) {
            continue;
        }
        game_set_condition(cid, CONDITION_GREAT);
        game_set_stamina(cid, (int16_t)game_get_max_stamina(cid));
    }
}

void p3p_day_condition_override(void) {
    p3p_apply_always_great();
    /* Clear daily morning condition-change notification flags without .rodata tables. */
    game_flag_set(2709, 0);
    game_flag_set(2710, 0);
    game_flag_set(2711, 0);
    game_flag_set(2712, 0);
    game_flag_set(2713, 0);
    game_flag_set(2714, 0);
    game_flag_set(2682, 0);
    game_flag_set(2715, 0);
    game_flag_set(2716, 0);
    game_flag_set(2717, 0);
    game_flag_set(2719, 0);
    game_flag_set(2765, 0);
    game_flag_set(2766, 0);
}
