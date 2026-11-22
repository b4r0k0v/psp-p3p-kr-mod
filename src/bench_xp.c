#include <stdint.h>

enum {
    XP_CAP = 65535u,
    MAX_LEVEL = 99u,
    MIN_COMPANION_ID = 2,
    MAX_COMPANION_ID = 10,
    FUUKA_ID = 6,
    MAX_PARTY_SLOTS = 4,
    MAX_TOAST_ENTRIES = 8,
    FADE_IN_US = 200000u,
    HOLD_END_US = 3400000u,
    TOTAL_US = 4000000u,
};

typedef struct {
    uint8_t cid;
    uint8_t old_lv;
    uint8_t new_lv;
    uint8_t pad;
} BenchToastEntry;

typedef struct {
    uint32_t count;
    uint32_t started;
    uint32_t started_us;
    BenchToastEntry entries[MAX_TOAST_ENTRIES];
} BenchToastState;

_Static_assert(sizeof(BenchToastState) <= 64, "Bench toast state exceeds allocation");

extern volatile uint32_t p3p_xp_mode;
extern volatile BenchToastState p3p_bench_toast_state;
extern uint8_t * volatile game_battle_state_ptr;
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
extern int game_is_female_route(void);
extern uint32_t game_time_us(void);
extern const uint32_t shrine_palettes[2][4];
extern void p3p_shrine_rect(uint32_t color, int x, int y, int width, int height);
extern void p3p_shrine_text(uint32_t color, int x, int y, const char *text);
extern const char bench_toast_title[];
extern const char bench_name_2[];
extern const char bench_name_3[];
extern const char bench_name_4[];
extern const char bench_name_5[];
extern const char bench_name_7[];
extern const char bench_name_8[];
extern const char bench_name_9[];
extern const char bench_name_10[];

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

static const char *bench_member_name(uint8_t cid)
{
    switch (cid) {
    case 2:  return bench_name_2;
    case 3:  return bench_name_3;
    case 4:  return bench_name_4;
    case 5:  return bench_name_5;
    case 7:  return bench_name_7;
    case 8:  return bench_name_8;
    case 9:  return bench_name_9;
    case 10: return bench_name_10;
    default: return bench_name_2;
    }
}

static int append_uint(char *buf, int pos, uint32_t val)
{
    if (val >= 10u)
        buf[pos++] = (char)('0' + (val / 10u) % 10u);
    buf[pos++] = (char)('0' + val % 10u);
    return pos;
}

void p3p_grant_bench_xp(const uint8_t *reward_struct, const uint8_t *battle_input)
{
    p3p_bench_toast_state.count = 0;
    p3p_bench_toast_state.started = 0;

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
        if (will_level_up && game_persona_can_level_up(rec)) {
            game_persona_apply_level_up(rec, lvlup);
            uint32_t new_lv = game_persona_level(rec);
            uint32_t count = p3p_bench_toast_state.count;
            if (new_lv > lv && count < MAX_TOAST_ENTRIES) {
                p3p_bench_toast_state.entries[count].cid = (uint8_t)cid;
                p3p_bench_toast_state.entries[count].old_lv = (uint8_t)lv;
                p3p_bench_toast_state.entries[count].new_lv = (uint8_t)new_lv;
                p3p_bench_toast_state.entries[count].pad = 0;
                p3p_bench_toast_state.count = count + 1u;
            }
        }
    }
}

void p3p_draw_bench_toast(void)
{
    uint32_t count = p3p_bench_toast_state.count;
    if (count == 0u || count > MAX_TOAST_ENTRIES)
        return;

    if (game_battle_state_ptr != 0)
        return;

    uint32_t now = game_time_us();
    if (!p3p_bench_toast_state.started) {
        p3p_bench_toast_state.started = 1u;
        p3p_bench_toast_state.started_us = now;
    }

    uint32_t elapsed = now - p3p_bench_toast_state.started_us;
    if (elapsed >= TOTAL_US) {
        p3p_bench_toast_state.count = 0u;
        p3p_bench_toast_state.started = 0u;
        return;
    }

    uint32_t alpha;
    if (elapsed < FADE_IN_US)
        alpha = (elapsed * 255u) / FADE_IN_US;
    else if (elapsed < HOLD_END_US)
        alpha = 255u;
    else
        alpha = ((TOTAL_US - elapsed) * 255u) / (TOTAL_US - HOLD_END_US);

    if (alpha == 0u)
        return;

    const uint32_t *pal = shrine_palettes[game_is_female_route() != 0];
    uint32_t bg_alpha = ((pal[0] & 0xffu) * alpha) / 255u;
    uint32_t bg = (pal[0] & 0xffffff00u) | bg_alpha;
    uint32_t accent_color = (pal[2] & 0xffffff00u) | alpha;
    uint32_t text_color = 0xffffff00u | alpha;

    int box_w = 166;
    int box_h = 22 + (int)count * 16;
    int box_x = 12;
    int box_y = 10;

    p3p_shrine_rect(bg, box_x, box_y, box_w, box_h);
    p3p_shrine_text(accent_color, box_x + 8, box_y + 4, bench_toast_title);

    for (uint32_t i = 0; i < count; ++i) {
        uint8_t cid = p3p_bench_toast_state.entries[i].cid;
        uint8_t old_lv = p3p_bench_toast_state.entries[i].old_lv;
        uint8_t new_lv = p3p_bench_toast_state.entries[i].new_lv;
        int y = box_y + 21 + (int)i * 16;

        p3p_shrine_text(text_color, box_x + 8, y, bench_member_name(cid));

        char buf[16];
        int pos = 0;
        buf[pos++] = 'L';
        buf[pos++] = 'v';
        buf[pos++] = '.';
        pos = append_uint(buf, pos, old_lv);
        buf[pos++] = ' ';
        buf[pos++] = '-';
        buf[pos++] = '>';
        buf[pos++] = ' ';
        pos = append_uint(buf, pos, new_lv);
        buf[pos] = 0;

        p3p_shrine_text(accent_color, box_x + 80, y, buf);
    }
}
