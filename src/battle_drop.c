#include <stdint.h>

typedef struct {
    uint16_t id;
    uint16_t qty;
} BattleDropEntry;

enum {
    MAX_VANILLA_DROPS = 3,
    MAX_EXTRA_DROPS = 29,
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
extern unsigned game_item_count(int item);
extern void game_item_set(int item, unsigned count);
extern const char *game_item_name(int item);
extern void p3p_shrine_text(uint32_t color, int x, int y, const char *text);

typedef struct {
    int32_t count;
    uint8_t *owner;
    BattleDropEntry items[MAX_EXTRA_DROPS];
} BattleDropState;

_Static_assert(sizeof(BattleDropState) <= 128, "Battle drop state exceeds its allocation");

extern volatile BattleDropState p3p_battle_drop_state;

static void add_drop_item(uint8_t *btl, uint16_t item_id)
{
    if (!item_id)
        return;

    BattleDropEntry *vanilla = (BattleDropEntry *)(btl + RESULT_ITEMS_OFFSET);
    int32_t *vanilla_count = (int32_t *)(btl + RESULT_COUNT_OFFSET);
    int count = *vanilla_count;
    if (count < 0)
        count = 0;
    else if (count > MAX_VANILLA_DROPS)
        count = MAX_VANILLA_DROPS;

    for (int i = 0; i < count; ++i) {
        if (vanilla[i].id == item_id) {
            if (vanilla[i].qty < ITEM_QTY_CAP)
                ++vanilla[i].qty;
            return;
        }
    }

    if (count < MAX_VANILLA_DROPS) {
        vanilla[count].id = item_id;
        vanilla[count].qty = 1;
        *vanilla_count = count + 1;
        return;
    }

    int extra = p3p_battle_drop_state.count;
    for (int i = 0; i < extra; ++i) {
        if (p3p_battle_drop_state.items[i].id == item_id) {
            if (p3p_battle_drop_state.items[i].qty < ITEM_QTY_CAP)
                ++p3p_battle_drop_state.items[i].qty;
            return;
        }
    }

    if (extra < MAX_EXTRA_DROPS) {
        p3p_battle_drop_state.items[extra].id = item_id;
        p3p_battle_drop_state.items[extra].qty = 1;
        p3p_battle_drop_state.count = extra + 1;
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
                add_drop_item(btl, ev_item);

            for (int i = 0; i < 4; ++i) {
                uint16_t item = *(const uint16_t *)(rec + 34 + i * 4);
                uint8_t rate = rec[36 + i * 4];
                if (!item || !rate)
                    continue;
                if (item >= CONDITIONAL_ITEM_START && game_flag_get(CONDITIONAL_FLAG_BASE - item))
                    continue;
                add_drop_item(btl, item);
            }
        }
    }

    if (slot) {
        uint16_t script_item = *(const uint16_t *)(slot + 0x80);
        uint8_t script_rate = slot[0x82];
        if (script_item && script_rate)
            add_drop_item(btl, script_item);
    }
}

void p3p_grant_extra_drops(int vanilla_count)
{
    int count = p3p_battle_drop_state.count;
    p3p_battle_drop_state.count = 0;
    p3p_battle_drop_state.owner = 0;
    if (vanilla_count != MAX_VANILLA_DROPS)
        return;

    for (int i = 0; i < count; ++i) {
        unsigned item = p3p_battle_drop_state.items[i].id;
        unsigned total = game_item_count(item) + p3p_battle_drop_state.items[i].qty;
        if (total > ITEM_QTY_CAP)
            total = ITEM_QTY_CAP;
        game_item_set(item, total);
    }
}

void p3p_draw_extra_drops(const uint8_t *ui)
{
    int count = p3p_battle_drop_state.count;
    if (count <= 0 || !ui)
        return;

    const uint8_t *state = *(const uint8_t *const *)(ui + 1576);
    if (!state || *(const int32_t *)(state + 60) != MAX_VANILLA_DROPS)
        return;

    uint32_t alpha = ui[1098];
    if (!alpha)
        return;

    uint32_t color = 0xffffff00u | alpha;
    int base_x = (int)*(const float *)(ui + 1088);
    int base_y = (int)*(const float *)(ui + 1092);
    int step_y = count <= 3 ? 19 : 15;
    int step_x = count <= 3 ? 18 : 14;

    for (int i = 0; i < count; ++i) {
        int x = base_x - step_x * (i + 1);
        int y = base_y + step_y * (i + 1);
        if (y > 258)
            break;

        unsigned item = p3p_battle_drop_state.items[i].id;
        unsigned qty = p3p_battle_drop_state.items[i].qty;
        if (qty > ITEM_QTY_CAP)
            qty = ITEM_QTY_CAP;

        p3p_shrine_text(color, x, y, game_item_name(item));

        char amount[6];
        int pos = 0;
        amount[pos++] = (char)0x82;
        amount[pos++] = (char)0xae; /* × */
        if (qty >= 10)
            amount[pos++] = '0' + qty / 10;
        amount[pos++] = '0' + qty % 10;
        amount[pos] = 0;
        p3p_shrine_text(color, x + 130, y, amount);
    }
}
