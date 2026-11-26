#include <stdint.h>

enum {
    FIRST_CARD = 0x690,
    CARD_LIMIT = 0x78f,
    PAGE_ROWS = 6,
    CARD_LOCAL = 9,
    ORDER_CARD = 93,
    ORDER_DAYS = 94,
    NO_CARDS = 439,
    CLOSE_DIALOGUE = 563,
    REGISTER_ORDER = 582,
    BUTTON_UP = 0x0010,
    BUTTON_DOWN = 0x0040,
    BUTTON_LEFT = 0x0080,
    BUTTON_RIGHT = 0x0020,
    BUTTON_CIRCLE = 0x2000,
    BUTTON_CROSS = 0x4000,
    INPUT_MASK = BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT | BUTTON_CIRCLE | BUTTON_CROSS,
    KEY_ICON_CIRCLE = 178,
    KEY_ICON_CROSS = 179,
    KEY_ICON_DPAD_LR = 454,
    KEY_ICON_SCALE = 0x0d00,
};

typedef struct {
    uint8_t *flow;
    uint32_t previous_edges;
    uint16_t cards[CARD_LIMIT - FIRST_CARD];
    uint16_t count;
    uint16_t cursor;
    uint8_t confirming;
    uint8_t presented;
} Selection;

typedef struct {
    uint32_t background;
    uint32_t highlight;
    uint32_t heading;
    uint32_t controls;
    uint32_t desc_bg;
} ShrinePalette;

_Static_assert(sizeof(Selection) <= 528, "Shrine selection exceeds its payload allocation");

extern Selection p3p_shrine_selection_state;
extern const ShrinePalette shrine_palettes[2];
extern volatile uint32_t game_button_edges;
extern int game_is_female_route(void);
extern int game_get_flow_variable(int index);
extern const uint8_t *game_item_record(int item);
extern unsigned game_item_count(int item);
extern const char *game_item_name(int item);
extern volatile int32_t game_skill_help_handle;
extern unsigned game_bmd_count(int handle);
extern void p3p_shrine_text(uint32_t color, int x, int y, const char *text);
extern void p3p_shrine_bmd_text(uint32_t color, int x, int y, int handle, int message_index);
extern void p3p_shrine_rect(uint32_t color, int x, int y, int width, int height);
extern void p3p_draw_key_icon(int sprite_id, int x, int y, int scale);
extern void game_play_se(int a0, int a1, int a2, int se_id);
extern const char shrine_list_title[], shrine_quantity[], shrine_confirm_title[],
    shrine_label_decide[], shrine_label_back[], shrine_label_page[],
    shrine_label_deposit[], shrine_label_cancel[], shrine_label_slash[];

static void refresh_cards(Selection *selection)
{
    selection->count = 0;
    for (int item = FIRST_CARD; item < CARD_LIMIT; ++item) {
        if (game_item_count(item))
            selection->cards[selection->count++] = item;
    }
    if (selection->cursor >= selection->count)
        selection->cursor = 0;
}

static int finish(Selection *selection, unsigned instruction)
{
    /* The COMM dispatcher pops the original random argument on success. */
    *(uint32_t *)(selection->flow + 0x1c) = instruction;
    selection->flow = 0;
    selection->confirming = 0;
    return 1;
}

static char *append_uint(char *out, unsigned value)
{
    if (value >= 10)
        *out++ = '0' + value / 10;
    *out++ = '0' + value % 10;
    return out;
}

static void draw_card(unsigned item, int y)
{
    unsigned quantity = game_item_count(item);
    char amount[3];
    amount[0] = quantity < 10 ? ' ' : '0' + quantity / 10;
    amount[1] = '0' + quantity % 10;
    amount[2] = 0;
    p3p_shrine_text(0xffffffff, 93, y, game_item_name(item));
    p3p_shrine_text(0xffffffff, 353, y, amount);
}

static void draw_card_description(unsigned item, uint32_t color, int x, int y)
{
    int handle = game_skill_help_handle;
    if (handle < 0)
        return;
    const uint8_t *record = game_item_record(item);
    if (!record)
        return;
    unsigned skill = *(const uint16_t *)(record + 0x24);
    if (skill && skill < game_bmd_count(handle))
        p3p_shrine_bmd_text(color, x, y, handle, skill);
}

void p3p_draw_panel(uint32_t bg, uint32_t border, int x, int y, int width, int height)
{
    p3p_shrine_rect(bg, x, y, width, height);
    p3p_shrine_rect(border, x, y, width, 1);
    p3p_shrine_rect(border, x, y + height - 1, width, 1);
    p3p_shrine_rect(border, x, y + 1, 1, height - 2);
    p3p_shrine_rect(border, x + width - 1, y + 1, 1, height - 2);
}

void p3p_draw_accent_row(uint32_t fill, uint32_t accent, int x, int y, int width, int height)
{
    p3p_shrine_rect(fill, x, y, width, height);
    p3p_shrine_rect(accent, x, y, 3, height);
}

void p3p_shrine_draw_selection(void)
{
    Selection *selection = &p3p_shrine_selection_state;
    const ShrinePalette *palette = &shrine_palettes[game_is_female_route() != 0];
    if (selection->confirming) {
        unsigned item = selection->cards[selection->cursor];
        p3p_draw_panel(palette->background, palette->highlight, 77, 58, 326, 156);
        p3p_shrine_text(palette->heading, 93, 68, shrine_confirm_title);
        p3p_shrine_rect(palette->highlight, 87, 87, 306, 1);
        p3p_draw_accent_row(palette->highlight, palette->heading, 87, 94, 306, 18);
        draw_card(item, 93);
        p3p_draw_panel(palette->desc_bg, palette->highlight, 87, 118, 306, 56);
        draw_card_description(item, palette->controls, 95, 123);
        p3p_draw_key_icon(KEY_ICON_CIRCLE, 93, 187, KEY_ICON_SCALE);
        p3p_shrine_text(palette->controls, 111, 186, shrine_label_deposit);
        p3p_shrine_text(palette->controls, 149, 186, shrine_label_slash);
        p3p_draw_key_icon(KEY_ICON_CROSS, 161, 187, KEY_ICON_SCALE);
        p3p_shrine_text(palette->controls, 179, 186, shrine_label_cancel);
        selection->presented = 1;
        return;
    }

    p3p_draw_panel(palette->background, palette->highlight, 77, 14, 326, 244);
    p3p_shrine_text(palette->heading, 93, 22, shrine_list_title);
    p3p_shrine_text(palette->heading, 349, 22, shrine_quantity);
    p3p_shrine_rect(palette->highlight, 87, 41, 306, 1);
    unsigned first_visible = selection->cursor / PAGE_ROWS * PAGE_ROWS;
    for (unsigned row = 0; row < PAGE_ROWS; ++row) {
        unsigned index = first_visible + row;
        if (index >= selection->count)
            break;
        int y = 46 + row * 18;
        if (index == selection->cursor)
            p3p_draw_accent_row(palette->highlight, palette->heading, 87, y + 1, 306, 17);
        draw_card(selection->cards[index], y);
    }
    p3p_draw_panel(palette->desc_bg, palette->highlight, 87, 160, 306, 62);
    draw_card_description(selection->cards[selection->cursor], palette->controls, 95, 165);

    unsigned total_pages = (selection->count + PAGE_ROWS - 1) / PAGE_ROWS;
    unsigned page = selection->cursor / PAGE_ROWS + 1;
    char page_text[8];
    char *p = append_uint(page_text, page);
    *p++ = '/';
    p = append_uint(p, total_pages);
    *p = 0;
    int page_w = (int)(p - page_text) * 9 - 1;
    int footer_x = (480 - (196 + page_w)) / 2;
    p3p_draw_key_icon(KEY_ICON_CIRCLE, footer_x, 233, KEY_ICON_SCALE);
    p3p_shrine_text(palette->controls, footer_x + 18, 232, shrine_label_decide);
    p3p_shrine_text(palette->controls, footer_x + 46, 232, shrine_label_slash);
    p3p_draw_key_icon(KEY_ICON_CROSS, footer_x + 58, 233, KEY_ICON_SCALE);
    p3p_shrine_text(palette->controls, footer_x + 76, 232, shrine_label_back);
    p3p_shrine_text(palette->controls, footer_x + 104, 232, shrine_label_slash);
    p3p_draw_key_icon(KEY_ICON_DPAD_LR, footer_x + 116, 232, KEY_ICON_SCALE);
    p3p_shrine_text(palette->controls, footer_x + 137, 232, shrine_label_page);
    p3p_shrine_text(palette->heading, footer_x + 196, 232, page_text);
    selection->presented = 1;
}

static void move_cursor(Selection *selection, unsigned input)
{
    uint16_t prev = selection->cursor;
    int cursor = prev;
    switch (input & (BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT)) {
    case BUTTON_UP: --cursor; break;
    case BUTTON_DOWN: ++cursor; break;
    case BUTTON_LEFT: cursor -= PAGE_ROWS; break;
    case BUTTON_RIGHT: cursor += PAGE_ROWS; break;
    default: return;
    }
    if (cursor < 0)
        cursor = input == BUTTON_UP ? selection->count - 1 : 0;
    else if (cursor >= selection->count)
        cursor = input == BUTTON_DOWN ? 0 : selection->count - 1;
    selection->cursor = cursor;
    if (selection->cursor != prev)
        game_play_se(0, 1, 0, 0);
}

int p3p_shrine_choose_card(uint8_t *flow)
{
    Selection *selection = &p3p_shrine_selection_state;
    unsigned edges = game_button_edges & INPUT_MASK;
    if (selection->flow != flow) {
        selection->flow = flow;
        selection->cursor = 0;
        selection->confirming = 0;
        selection->presented = 0;
        selection->previous_edges = edges;
        refresh_cards(selection);
        if (!selection->count)
            return finish(selection, NO_CARDS);
    }

    unsigned input = edges & ~selection->previous_edges;
    selection->previous_edges = edges;
    if (input & BUTTON_CROSS) {
        game_play_se(0, 0, 0, 2);
        if (!selection->confirming)
            return finish(selection, CLOSE_DIALOGUE);
        selection->confirming = 0;
        selection->presented = 0;
    } else if (!selection->presented) {
        return 0;
    } else if (input & BUTTON_CIRCLE) {
        if (selection->confirming) {
            unsigned item = selection->cards[selection->cursor];
            if (game_get_flow_variable(ORDER_CARD) || game_get_flow_variable(ORDER_DAYS)) {
                game_play_se(0, 0, 0, 2);
                return finish(selection, CLOSE_DIALOGUE);
            }
            if (game_item_count(item)) {
                int32_t *locals = *(int32_t **)(flow + 0x144);
                locals[CARD_LOCAL] = item;
                game_play_se(0, 0, 0, 1);
                return finish(selection, REGISTER_ORDER);
            }
            game_play_se(0, 0, 0, 2);
            selection->confirming = 0;
            selection->presented = 0;
            refresh_cards(selection);
            if (!selection->count)
                return finish(selection, NO_CARDS);
        } else {
            game_play_se(0, 0, 0, 1);
            selection->confirming = 1;
            selection->presented = 0;
        }
    } else if (!selection->confirming) {
        move_cursor(selection, input);
    }
    return 0;
}
