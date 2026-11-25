#include <stdint.h>

enum {
    SKILL_SLOTS = 8,
    MAX_CANDIDATES = 6 * SKILL_SLOTS,
    /* Twelve regular results and the separate special-fusion result fit together. */
    CACHED_RESULTS = 16,
    BUTTON_LEFT = 0x0080,
    BUTTON_RIGHT = 0x0020,
    BUTTON_UP = 0x0010,
    BUTTON_DOWN = 0x0040,
    BUTTON_CIRCLE = 0x2000,
    BUTTON_CROSS = 0x4000,
    STOCK_HELP_VISIBLE = 0x0800,
};

typedef uint8_t Byte;

/* Game object offsets and their observed layouts are in reverse/ucks45140-fusion.md. */
typedef struct {
    const Byte *result;
    uint16_t persona;
    uint16_t candidates[MAX_CANDIDATES];
    uint8_t count;
    uint8_t limit;
} InheritancePool;

typedef struct {
    InheritancePool pools[CACHED_RESULTS];
    unsigned next_pool;
    const Byte *context;
    Byte *display;
    Byte *result;
    const InheritancePool *pool;
    uint16_t skills[SKILL_SLOTS];
    uint16_t inherited[SKILL_SLOTS];
    uint8_t slots[SKILL_SLOTS];
    uint8_t selected[SKILL_SLOTS];
    uint8_t cursor;
    uint8_t chosen;
    uint8_t limit;
} InheritanceState;

_Static_assert(sizeof(InheritanceState) <= 2048, "Fusion payload state exceeds its allocation");

extern InheritanceState p3p_fusion_state;
extern volatile uint32_t game_fusion_button_edges;
extern void game_fusion_open_help(Byte *owner);
extern void game_fusion_close_help(Byte *owner, int play_sound);
extern void game_play_se(int a0, int a1, int a2, int se_id);
extern int game_is_female_route(void);
extern void p3p_shrine_rect(uint32_t color, int x, int y, int width, int height);
extern void p3p_shrine_text(uint32_t color, int x, int y, const char *text);
extern void p3p_draw_key_icon(int sprite_id, int x, int y, int scale);
extern const char fusion_inherit[], fusion_prev[];

static uint16_t read_half(const Byte *object, unsigned offset)
{
    return *(const uint16_t *)(object + offset);
}

static void write_half(Byte *object, unsigned offset, uint16_t value)
{
    *(uint16_t *)(object + offset) = value;
}

static Byte *read_pointer(const Byte *object, unsigned offset)
{
    return (Byte *)(uintptr_t)*(const uint32_t *)(object + offset);
}

static int stock_help_visible(const Byte *display)
{
    return read_half(display, 0x520) & STOCK_HELP_VISIBLE;
}

static void show_candidate_help(const InheritanceState *state)
{
    Byte *display = state->display;
    const uint8_t slot = state->slots[state->chosen];
    if (display[0xa0] != slot) {
        display[0xa1] = display[0xa0];
        display[0xa0] = slot;
    }
}

/* The lottery owns this list and releases it before the preview opens. */
void p3p_fusion_record_pool(const Byte *persona, const Byte *head, unsigned limit)
{
    InheritanceState *state = &p3p_fusion_state;
    const Byte *result = persona - 4;
    unsigned index = 0;
    while (index < CACHED_RESULTS && state->pools[index].result != result)
        ++index;
    if (index == CACHED_RESULTS) {
        index = state->next_pool;
        state->next_pool = (index + 1) % CACHED_RESULTS;
    }

    InheritancePool *pool = &state->pools[index];
    pool->result = result;
    pool->persona = read_half(persona, 2);
    pool->count = 0;
    pool->limit = limit;
    for (const Byte *node = read_pointer(head, 12); node; node = read_pointer(node, 12)) {
        if (pool->count == MAX_CANDIDATES) {
            pool->count = 0;
            return;
        }
        pool->candidates[pool->count++] = read_half(node, 20);
    }
}

static const InheritancePool *find_pool(const Byte *result)
{
    InheritanceState *state = &p3p_fusion_state;
    const uint16_t persona = read_half(result, 6);
    for (unsigned index = 0; index < CACHED_RESULTS; ++index) {
        const InheritancePool *pool = &state->pools[index];
        if (pool->result == result && pool->persona == persona)
            return pool;
    }
    return 0;
}

static int begin_selection(Byte *context, Byte *display, Byte *result)
{
    InheritanceState *state = &p3p_fusion_state;
    const InheritancePool *pool = find_pool(result);
    if (!pool || !pool->count)
        return 0;

    unsigned slot_count = 0;
    for (unsigned slot = 0; slot < SKILL_SLOTS; ++slot) {
        const uint16_t skill = read_half(result, 0x10 + slot * 2);
        const uint16_t inherited = read_half(result, 0x3e + slot * 2);
        state->skills[slot] = skill;
        state->inherited[slot] = inherited;
        if (!inherited)
            continue;
        unsigned index = 0;
        while (index < pool->count && pool->candidates[index] != inherited)
            ++index;
        if (index == pool->count || skill != inherited)
            return 0;
        state->slots[slot_count++] = slot;
    }
    if (!slot_count || slot_count > pool->limit || slot_count > pool->count)
        return 0;

    state->context = context;
    state->display = display;
    state->result = result;
    state->pool = pool;
    state->chosen = 0;
    state->cursor = 0;
    state->limit = slot_count;
    return 1;
}

static void display_skills(int selecting)
{
    InheritanceState *state = &p3p_fusion_state;
    unsigned count = 0;
    unsigned inherited_mask = 0;
    for (unsigned slot = 0, position = 0; slot < SKILL_SLOTS; ++slot) {
        uint16_t skill = state->skills[slot];
        uint16_t inherited = state->inherited[slot];
        if (selecting && inherited) {
            if (position < state->chosen)
                skill = state->pool->candidates[state->selected[position]];
            else if (position == state->chosen)
                skill = state->pool->candidates[state->cursor];
            else
                skill = 0;
            inherited = skill;
            ++position;
        }
        write_half(state->display, 0xa6 + slot * 12, skill);
        if (skill)
            count = slot + 1;
        if (inherited)
            inherited_mask |= 1 << slot;
    }
    write_half(state->display, 0x104, count);
    state->display[0x106] = inherited_mask;
}

static void move_cursor(int direction)
{
    InheritanceState *state = &p3p_fusion_state;
    const int count = state->pool->count;
    for (;;) {
        int cursor = state->cursor + direction;
        if (cursor < 0)
            cursor = count - 1;
        else if (cursor == count)
            cursor = 0;
        state->cursor = cursor;
        unsigned position = 0;
        while (position < state->chosen && state->selected[position] != state->cursor)
            ++position;
        if (position == state->chosen)
            return;
    }
}

int p3p_fusion_preview_input(Byte *context)
{
    InheritanceState *state = &p3p_fusion_state;
    const uint32_t buttons = game_fusion_button_edges;
    Byte *result = read_pointer(context, 0xa70);
    Byte *owner = read_pointer(context, 0);
    Byte *display = owner ? read_pointer(owner, 0x38) : 0;

    /* The preview input handler also handles the material tabs. */
    if (!display || !result || read_pointer(display, 0) != result + 4 ||
        read_half(context, 0xa76) + read_half(context, 0xa78) + 1 != read_half(context, 0xa74)) {
        state->context = 0;
        return 0;
    }

    if (state->context != context || state->display != display || state->result != result) {
        state->context = 0;
        if (!(buttons & BUTTON_CIRCLE))
            return 0;
        if (stock_help_visible(display))
            return 1;
        if ((result[0] & 2) ||
            !begin_selection(context, display, result))
            return 0;
        display_skills(1);
        game_fusion_open_help(owner);
        show_candidate_help(state);
        return 1;
    }

    if (buttons & BUTTON_CROSS) {
        game_play_se(0, 0, 0, 2);
        if (state->chosen > 0) {
            state->cursor = state->selected[--state->chosen];
            display_skills(1);
            show_candidate_help(state);
        } else {
            game_fusion_close_help(owner, 0);
            display_skills(0);
            state->context = 0;
        }
    } else if (buttons & BUTTON_CIRCLE) {
        state->selected[state->chosen++] = state->cursor;
        if (state->chosen == state->limit) {
            display_skills(1);
            for (unsigned position = 0; position < state->limit; ++position) {
                const unsigned slot = state->slots[position];
                const uint16_t skill = state->pool->candidates[state->selected[position]];
                write_half(result, 0x10 + slot * 2, skill);
                write_half(result, 0x3e + slot * 2, skill);
            }
            /* Let this Circle press open the game's confirmation and close skill help. */
            state->context = 0;
            return 0;
        }
        game_play_se(0, 0, 0, 1);
        move_cursor(1);
        display_skills(1);
        show_candidate_help(state);
    } else if (buttons & (BUTTON_LEFT | BUTTON_UP | BUTTON_RIGHT | BUTTON_DOWN)) {
        uint8_t prev = state->cursor;
        move_cursor(buttons & (BUTTON_LEFT | BUTTON_UP) ? -1 : 1);
        if (state->cursor != prev)
            game_play_se(0, 1, 0, 0);
        display_skills(1);
        show_candidate_help(state);
    }
    return 1;
}

/* The game draws its stock footer once for the current preview context. */
int p3p_fusion_is_selecting(const Byte *context)
{
    return p3p_fusion_state.context == context;
}

int p3p_fusion_draw_help_guide(const Byte *context)
{
    const InheritanceState *state = &p3p_fusion_state;
    if (state->context != context)
        return 0;

    const int female = game_is_female_route() != 0;
    const uint32_t bg = female ? 0x3b1228e0 : 0x07233fe0;
    const uint32_t accent = female ? 0xff8ab8ff : 0x5eb8f2ff;
    const uint32_t divider = female ? 0xff8ab888 : 0x5eb8f288;
    const uint32_t label_color = female ? 0xffc8dfff : 0x9ce2ffff;
    const uint32_t sub_color = female ? 0xffe2eeff : 0xd8f0ffff;

    char progress[4];
    progress[0] = '0' + state->chosen + 1;
    progress[1] = '/';
    progress[2] = '0' + state->limit;
    progress[3] = 0;

    const int width = state->chosen ? 128 : 74;
    p3p_shrine_rect(bg, 21, 251, width, 18);
    p3p_shrine_rect(accent, 21, 251, 3, 18);
    /* Draw each token separately so 0x20 does not render as a dot glyph. */
    p3p_shrine_text(label_color, 31, 250, fusion_inherit);
    p3p_shrine_text(0xffffffff, 58, 250, progress);
    if (state->chosen) {
        p3p_shrine_rect(divider, 94, 254, 1, 12);
        p3p_draw_key_icon(179, 100, 252, 0x0c00);
        p3p_shrine_text(sub_color, 118, 250, fusion_prev);
    }
    return 1;
}
