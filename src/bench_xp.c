#include <stdint.h>

enum {
    XP_CAP = 65535u,
    MAX_LEVEL = 99u,
    MIN_COMPANION_ID = 2,
    MAX_COMPANION_ID = 10,
    FUUKA_ID = 6,
    MAX_PARTY_SLOTS = 4,
};

extern volatile uint32_t p3p_xp_mode;
extern int game_is_member_available(int char_id);
extern uint8_t *game_companion_record(int char_id);
extern uint32_t game_persona_level(const uint8_t *rec);
extern uint32_t game_persona_exp(const uint8_t *rec);
extern uint32_t game_persona_exp_for_level(const uint8_t *rec, uint32_t lv);
extern int32_t game_calc_base_xp(uint32_t lv, const uint8_t *battle_input);
extern void game_persona_calc_level_up(uint8_t *rec, uint8_t *out_lvlup, int32_t delta_exp);
extern void game_persona_add_exp(uint8_t *rec, int32_t delta_exp);
extern int game_persona_can_level_up(const uint8_t *rec);
extern void game_persona_apply_level_up(uint8_t *rec, const uint8_t *lvlup);

static int32_t scale_xp(int32_t xp)
{
    if (xp <= 0)
        return 0;

    uint32_t value = (uint32_t)xp;
    if (value > XP_CAP)
        value = XP_CAP;

    uint32_t mode = p3p_xp_mode;
    if (mode >= 1u && mode <= 4u) {
        if (mode == 1u)
            value += value >> 1;
        else
            value <<= (mode - 1u);

        if (value > XP_CAP)
            value = XP_CAP;
    }

    return (int32_t)value;
}

static int is_in_list(int cid, const uint16_t *ids, int count)
{
    for (int i = 0; i < count; ++i) {
        if (ids[i] == (uint16_t)cid)
            return 1;
    }
    return 0;
}

void p3p_grant_bench_xp(const uint8_t *reward_struct, const uint8_t *battle_input)
{
    if (!reward_struct || !battle_input)
        return;

    if (*(const int32_t *)(battle_input + 4) <= 0)
        return;

    int party_count = *(const int32_t *)(battle_input + 32);
    if (party_count < 0)
        party_count = 0;
    else if (party_count > MAX_PARTY_SLOTS)
        party_count = MAX_PARTY_SLOTS;

    const uint16_t *participants = (const uint16_t *)(battle_input + 22);
    const uint16_t *vanilla_ids = (const uint16_t *)(reward_struct + 0x69A);

    for (int cid = MIN_COMPANION_ID; cid <= MAX_COMPANION_ID; ++cid) {
        if (cid == FUUKA_ID)
            continue;
        if (is_in_list(cid, participants, party_count))
            continue;
        if (is_in_list(cid, vanilla_ids, MAX_PARTY_SLOTS))
            continue;
        if (!game_is_member_available(cid))
            continue;

        uint8_t *rec = game_companion_record(cid);
        if (!rec)
            continue;

        uint32_t lv = game_persona_level(rec);
        if (lv >= MAX_LEVEL)
            continue;

        int32_t exp = scale_xp(game_calc_base_xp(lv, battle_input));
        if (exp <= 0)
            continue;

        uint8_t lvlup[136];
        int will_level_up = 0;
        uint32_t cur_exp = game_persona_exp(rec);
        uint32_t next_exp = game_persona_exp_for_level(rec, lv + 1u);
        if (cur_exp + (uint32_t)exp >= next_exp) {
            game_persona_calc_level_up(rec, lvlup, exp);
            will_level_up = 1;
        }

        game_persona_add_exp(rec, exp);
        if (will_level_up && game_persona_can_level_up(rec))
            game_persona_apply_level_up(rec, lvlup);
    }
}
