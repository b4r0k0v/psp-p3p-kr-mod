#include <stdint.h>

typedef struct {
    uint16_t id;
    uint16_t qty;
} BattleDropEntry;

enum {
    MAX_VANILLA_DROPS = 3,
    MAX_CANDIDATES = 13,
    ENEMY_COUNT = 336,
    ENEMY_RECORD_SIZE = 62,
    ENEMY_UNIT_FLAG = 4,
    CONDITIONAL_ITEM_START = 0x600,
    CONDITIONAL_FLAG_BASE = 0x800,
    ITEM_QTY_CAP = 99,
    RESULT_ITEMS_OFFSET = 0xc84,
    RESULT_COUNT_OFFSET = 0xc90,
};

extern const uint8_t * volatile game_enemy_table_ptr;
extern uint8_t * volatile game_battle_state_ptr;
extern int game_flag_get(int flag);

typedef struct {
    uint16_t id;
    uint16_t qty;
    uint16_t key;
    uint8_t priority; /* 1 = 이벤트/스크립트 드롭, 0 = 일반 드롭 */
    uint8_t pad;
} BattleDropCandidate;

typedef struct {
    int32_t count;
    uint8_t *owner;
    uint32_t rng;
    uint32_t pad;
    BattleDropCandidate items[MAX_CANDIDATES];
} BattleDropState;

_Static_assert(sizeof(BattleDropState) <= 128, "Battle drop state exceeds its allocation");

extern volatile BattleDropState p3p_battle_drop_state;

static uint16_t next_rand16(void)
{
    uint32_t x = p3p_battle_drop_state.rng;
    if (!x)
        x = 0x9e3779b9u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    p3p_battle_drop_state.rng = x;
    return (uint16_t)(x >> 16);
}

static void add_candidate(uint16_t item_id, uint8_t priority)
{
    if (!item_id)
        return;

    int count = p3p_battle_drop_state.count;
    for (int i = 0; i < count; ++i) {
        if (p3p_battle_drop_state.items[i].id == item_id) {
            if (p3p_battle_drop_state.items[i].qty < ITEM_QTY_CAP)
                ++p3p_battle_drop_state.items[i].qty;
            if (priority)
                p3p_battle_drop_state.items[i].priority = 1;
            return;
        }
    }

    if (count < MAX_CANDIDATES) {
        p3p_battle_drop_state.items[count].id = item_id;
        p3p_battle_drop_state.items[count].qty = 1;
        p3p_battle_drop_state.items[count].key = next_rand16();
        p3p_battle_drop_state.items[count].priority = priority;
        p3p_battle_drop_state.items[count].pad = 0;
        p3p_battle_drop_state.count = count + 1;
        return;
    }

    if (priority) {
        int replace_idx = -1;
        uint16_t max_key = 0;
        for (int i = 0; i < count; ++i) {
            if (p3p_battle_drop_state.items[i].priority == 0 &&
                (replace_idx < 0 || p3p_battle_drop_state.items[i].key > max_key)) {
                replace_idx = i;
                max_key = p3p_battle_drop_state.items[i].key;
            }
        }
        if (replace_idx >= 0) {
            p3p_battle_drop_state.items[replace_idx].id = item_id;
            p3p_battle_drop_state.items[replace_idx].qty = 1;
            p3p_battle_drop_state.items[replace_idx].key = next_rand16();
            p3p_battle_drop_state.items[replace_idx].priority = 1;
            p3p_battle_drop_state.items[replace_idx].pad = 0;
        }
    }
}

static void sync_vanilla_slots(uint8_t *btl)
{
    BattleDropEntry *vanilla = (BattleDropEntry *)(btl + RESULT_ITEMS_OFFSET);
    int32_t *vanilla_count = (int32_t *)(btl + RESULT_COUNT_OFFSET);
    int count = p3p_battle_drop_state.count;
    int filled = 0;

    for (int i = 0; i < count && filled < MAX_VANILLA_DROPS; ++i) {
        if (p3p_battle_drop_state.items[i].priority) {
            vanilla[filled].id = p3p_battle_drop_state.items[i].id;
            vanilla[filled].qty = p3p_battle_drop_state.items[i].qty;
            ++filled;
        }
    }

    if (filled < MAX_VANILLA_DROPS) {
        int normal_count = 0;
        for (int i = 0; i < count; ++i) {
            if (p3p_battle_drop_state.items[i].priority == 0)
                ++normal_count;
        }

        int rem = MAX_VANILLA_DROPS - filled;
        if (normal_count <= rem) {
            for (int i = 0; i < count && filled < MAX_VANILLA_DROPS; ++i) {
                if (p3p_battle_drop_state.items[i].priority == 0) {
                    vanilla[filled].id = p3p_battle_drop_state.items[i].id;
                    vanilla[filled].qty = p3p_battle_drop_state.items[i].qty;
                    ++filled;
                }
            }
        } else {
            uint32_t picked = 0;
            for (int step = 0; step < rem; ++step) {
                int best = -1;
                uint16_t min_key = 0xffffu;
                for (int i = 0; i < count; ++i) {
                    if (p3p_battle_drop_state.items[i].priority != 0)
                        continue;
                    if (picked & (1u << i))
                        continue;
                    if (best < 0 || p3p_battle_drop_state.items[i].key < min_key) {
                        best = i;
                        min_key = p3p_battle_drop_state.items[i].key;
                    }
                }
                if (best < 0)
                    break;
                picked |= (1u << best);
                vanilla[filled].id = p3p_battle_drop_state.items[best].id;
                vanilla[filled].qty = p3p_battle_drop_state.items[best].qty;
                ++filled;
            }
        }
    }

    *vanilla_count = filled;
    for (int i = filled; i < MAX_VANILLA_DROPS; ++i) {
        vanilla[i].id = 0;
        vanilla[i].qty = 0;
    }
}

void p3p_collect_enemy_drops(const uint8_t *slot, const uint8_t *unit_data)
{
    uint8_t *btl = game_battle_state_ptr;
    if (!btl)
        return;

    if (*(int32_t *)(btl + RESULT_COUNT_OFFSET) == 0 || p3p_battle_drop_state.owner != btl) {
        p3p_battle_drop_state.count = 0;
        p3p_battle_drop_state.owner = btl;
        p3p_battle_drop_state.rng ^= (uint32_t)(uintptr_t)btl + 0x9e3779b9u;
    }

    if (unit_data && (*(const uint16_t *)unit_data & ENEMY_UNIT_FLAG)) {
        uint16_t enemy_id = *(const uint16_t *)(unit_data + 2);
        const uint8_t *table = game_enemy_table_ptr;
        if (table && enemy_id < ENEMY_COUNT) {
            const uint8_t *rec = table + enemy_id * ENEMY_RECORD_SIZE;
            uint16_t ev_flag = *(const uint16_t *)(rec + 50);
            uint16_t ev_item = *(const uint16_t *)(rec + 52);
            uint8_t ev_rate = rec[54];
            if (ev_item && ev_flag && ev_rate && game_flag_get(ev_flag))
                add_candidate(ev_item, 1);

            for (int i = 0; i < 4; ++i) {
                uint16_t item = *(const uint16_t *)(rec + 34 + i * 4);
                uint8_t rate = rec[36 + i * 4];
                if (!item || !rate)
                    continue;
                if (item >= CONDITIONAL_ITEM_START && game_flag_get(CONDITIONAL_FLAG_BASE - item))
                    continue;
                add_candidate(item, 0);
            }
        }
    }

    if (slot) {
        uint16_t script_item = *(const uint16_t *)(slot + 0x80);
        uint8_t script_rate = slot[0x82];
        if (script_item && script_rate)
            add_candidate(script_item, 1);
    }

    sync_vanilla_slots(btl);
}
