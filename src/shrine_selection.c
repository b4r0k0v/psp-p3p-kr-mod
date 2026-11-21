#include <stdint.h>

enum {
    FIRST_CARD = 0x690,
    CARD_LIMIT = 0x78f,
    PAGE_ROWS = 8,
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

_Static_assert(sizeof(Selection) <= 528, "Shrine selection exceeds its payload allocation");

extern Selection p3p_shrine_selection_state;
extern volatile uint32_t game_button_edges;
extern int game_get_flow_variable(int index);
extern unsigned game_item_count(int item);
extern const char *game_item_name(int item);
extern void p3p_shrine_text(uint32_t color, int x, int y, const char *text);
extern void p3p_shrine_rect(uint32_t color, int x, int y, int width, int height);
extern const char shrine_list_title[], shrine_quantity[], shrine_confirm_title[], shrine_controls[];

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

static void draw_card(unsigned item, int y)
{
    unsigned quantity = game_item_count(item);
    char amount[3];
    amount[0] = quantity < 10 ? ' ' : '0' + quantity / 10;
    amount[1] = '0' + quantity % 10;
    amount[2] = 0;
    p3p_shrine_text(0xffffffff, 117, y, game_item_name(item));
    p3p_shrine_text(0xffffffff, 329, y, amount);
}

void p3p_shrine_draw_selection(void)
{
    Selection *selection = &p3p_shrine_selection_state;
    if (selection->confirming) {
        p3p_shrine_rect(0x17233ef8, 97, 84, 286, 105);
        p3p_shrine_text(0xfdd37dff, 117, 97, shrine_confirm_title);
        draw_card(selection->cards[selection->cursor], 126);
        p3p_shrine_text(0xd7def1ff, 117, 160, shrine_controls);
        selection->presented = 1;
        return;
    }

    p3p_shrine_rect(0x17233ef8, 97, 24, 286, 224);
    p3p_shrine_text(0xfdd37dff, 117, 35, shrine_list_title);
    p3p_shrine_text(0xfdd37dff, 325, 35, shrine_quantity);
    unsigned first_visible = selection->cursor / PAGE_ROWS * PAGE_ROWS;
    for (unsigned row = 0; row < PAGE_ROWS; ++row) {
        unsigned index = first_visible + row;
        if (index >= selection->count)
            break;
        int y = 61 + row * 19;
        if (index == selection->cursor)
            p3p_shrine_rect(0x42577fff, 107, y - 3, 266, 19);
        draw_card(selection->cards[index], y);
    }
    p3p_shrine_text(0xd7def1ff, 117, 220, shrine_controls);
    selection->presented = 1;
}

static void move_cursor(Selection *selection, unsigned input)
{
    int cursor = selection->cursor;
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
        if (!selection->confirming)
            return finish(selection, CLOSE_DIALOGUE);
        selection->confirming = 0;
        selection->presented = 0;
    } else if (!selection->presented) {
        return 0;
    } else if (input & BUTTON_CIRCLE) {
        if (selection->confirming) {
            unsigned item = selection->cards[selection->cursor];
            if (game_get_flow_variable(ORDER_CARD) || game_get_flow_variable(ORDER_DAYS))
                return finish(selection, CLOSE_DIALOGUE);
            if (game_item_count(item)) {
                int32_t *locals = *(int32_t **)(flow + 0x144);
                locals[CARD_LOCAL] = item;
                return finish(selection, REGISTER_ORDER);
            }
            selection->confirming = 0;
            selection->presented = 0;
            refresh_cards(selection);
            if (!selection->count)
                return finish(selection, NO_CARDS);
        } else {
            selection->confirming = 1;
            selection->presented = 0;
        }
    } else if (!selection->confirming) {
        move_cursor(selection, input);
    }
    return 0;
}
