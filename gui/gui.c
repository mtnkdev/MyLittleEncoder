#include "gui.h"
#include "list.h"
#include "keys.h"
#include <curses.h>
#include <time.h>
#include <stdio.h>

/* color_pair indices */
#define TITLECOLOR          1
#define MAINMENUCOLOR       (2 | A_BOLD)
#define MAINMENUREVCOLOR    (3 | A_BOLD | A_REVERSE)
#define SUBMENUCOLOR        (4 | A_BOLD)
#define SUBMENUREVCOLOR     (5 | A_BOLD | A_REVERSE)
#define BODYCOLOR           6
#define BODYREVCOLOR        (7 | A_REVERSE)
#define STATUSCOLOR         (8 | A_BOLD)
#define INPUTBOXCOLOR       9
#define EDITBOXCOLOR        (10 | A_BOLD | A_REVERSE)

/* dimensions */
#define TH 1
#define MH 1
#define SH 2
#define BH (LINES - TH - MH - SH)
#define BW COLS

/* macro */
#define min(a,b) (a) < (b) ? (a) : (b)

/************************* PREDEFINITIONS *************************/
void status_message(char*);
void error_message(char*, int);
void remove_status_message();
void remove_error_message();
int wait_for_key();

/***************************** STATIC *****************************/
static WINDOW *wtitle, *wmain, *wbody, *wstatus;
static bool quit = FALSE;
static bool in_curses = FALSE;
static int key = ERR, ch = ERR;
static int menux, menuy;

/*********************** INTERNAL FUNCTIONS ***********************/
static void init_colors() {
    start_color();

    init_pair(TITLECOLOR       & ~A_ATTR, COLOR_BLACK, COLOR_CYAN);
    init_pair(MAINMENUCOLOR    & ~A_ATTR, COLOR_WHITE, COLOR_CYAN);
    init_pair(MAINMENUREVCOLOR & ~A_ATTR, COLOR_WHITE, COLOR_BLUE); /// COLOR_WHITE, COLOR_BLACK
    init_pair(SUBMENUCOLOR     & ~A_ATTR, COLOR_WHITE, COLOR_CYAN);
    init_pair(SUBMENUREVCOLOR  & ~A_ATTR, COLOR_WHITE, COLOR_BLUE); /// COLOR_WHITE, COLOR_BLACK
    init_pair(BODYCOLOR        & ~A_ATTR, COLOR_WHITE, COLOR_BLUE);
    init_pair(BODYREVCOLOR     & ~A_ATTR, COLOR_WHITE, COLOR_BLACK);
    init_pair(STATUSCOLOR      & ~A_ATTR, COLOR_WHITE, COLOR_CYAN);
    init_pair(INPUTBOXCOLOR    & ~A_ATTR, COLOR_BLACK, COLOR_CYAN);
    init_pair(EDITBOXCOLOR     & ~A_ATTR, COLOR_WHITE, COLOR_BLACK);
}

static void set_color(WINDOW *win, chtype color) {
    chtype attr = color & A_ATTR;
    attr &= ~A_REVERSE; // ignore reverse
    wattrset(win, COLOR_PAIR(color & A_CHARTEXT) | attr);
}

static void color_box(WINDOW *win, chtype color, int has_box) {
    chtype attr = color & A_ATTR;
    set_color(win, color);

    wbkgd(win, COLOR_PAIR(color & A_CHARTEXT) | (attr & ~A_REVERSE));
    werase(win);

    if (has_box && getmaxy(win) > 2)
        box(win, 0, 0);

    touchwin(win);
    wrefresh(win);
}

static char* pad_str_left(char *s, int length) {
    static char buffer[MAX_STR_LEN];
    char format[10];

    sprintf(format, strlen(s) > length ? "%%.%ds" : "%%-%ds", length);
    sprintf(buffer, format, s);

    return buffer;
}

/// assumes enough space
static char* str_add_space_left(char *s) {
    int len = strlen(s);
    int i;

    for (i = len; i; i--)
        s[i] = s[i - 1];
    s[len + 1] = '\0';
    s[0] = ' ';

    return s;
}

static char* convert_key_to_string(Key *key) {
    static char s[MAX_KEY_ROW_LEN];
    sprintf(s, KEY_PRINT_FORMAT, key->type, key->mode, key->key_name, key->key);
    return s;
}

static void remove_line(WINDOW *win, int line) {
    mvwaddstr(win, line, 1, pad_str_left(" ", BW - 2));
    wrefresh(win);
}

static void idle() {
    char buffer[MAX_STR_LEN];
    time_t t;
    struct tm *tp;

    if (time(&t) == -1)
        return;

    tp = localtime(&t);
    sprintf(buffer, "%.2d-%.2d-%.4d  %.2d:%.2d:%.2d", tp->tm_mday, tp->tm_mon + 1,
            tp->tm_year + 1900, tp->tm_hour, tp->tm_min, tp->tm_sec);

    // mvwaddstr(wtitle, 0, BW - strlen(buffer) - 2, buffer);
    mvwaddstr(wtitle, 0, BW - 22, buffer);
    wrefresh(wtitle);
}

static void menu_dimensions(Menu *mp, int *lines, int *columns) {
    int i, len, max = 0;

    for (i = 0; mp->func; i++, mp++)
        if ((len = strlen(mp->name)) > max)
            max = len;

    *lines = i;
    *columns = max + 2;
}

static void set_menu_position(int y, int x) {
    menuy = y;
    menux = x;
}

static void get_menu_position(int *y, int *x) {
    *y = menuy;
    *x = menux;
}

static void repaint_menu(WINDOW *wmenu, Menu *mp) {
    int i;

    for (i = 0; mp->func; i++, mp++)
        mvwaddstr(wmenu, i + 1, 2, mp->name);

    touchwin(wmenu);
    wrefresh(wmenu);
}

static void repaint_main_menu(Menu *mp, int width) {
    int i;

    for (i = 0; mp->func; i++, mp++)
        mvwaddstr(wmain, 0, i * width, str_add_space_left(pad_str_left(mp->name, width - 1)));

    touchwin(wmain);
    wrefresh(wmain);
}

static void repaint_body_keys(ListElement *list_elem, int length, int pad_y, int pad_x) {
    int i;
    ListElement *curr = list_elem;
    char row[MAX_KEY_ROW_LEN];

    sprintf(row, KEY_PRINT_FORMAT, "Type", "Mode", "Key name", "Key");
    ///mvwaddstr(wbody, pad_y, pad_x, row);
    error_message(row, 0);

    for (i = 0; curr && i < length; i++, curr = curr->next)
        mvwaddstr(wbody, i + pad_y, pad_x, convert_key_to_string((Key*)curr->info));

    touchwin(wbody);
    wrefresh(wbody);
}

static void main_help() {
#ifdef ALT_X
    status_message("Use arrow keys and Enter to select (Alt-X to quit)");
#else
    status_message("Use arrow keys and Enter to select");
#endif // ALT_X
}

static void main_menu(Menu *mp) {
    int n_items, bar_len;
    int old_selection = -1, curr_selection = 0;
    int c;

    menu_dimensions(mp, &n_items, &bar_len);
    repaint_main_menu(mp, bar_len);

    while (!quit) {
        if (curr_selection != old_selection) {
            if (old_selection != -1) {
                mvwaddstr(wmain, 0, old_selection * bar_len, str_add_space_left(pad_str_left(mp[old_selection].name, bar_len - 1)));
                status_message(mp[curr_selection].description);
            }
            else
                main_help();

            set_color(wmain, MAINMENUREVCOLOR);
            mvwaddstr(wmain, 0, curr_selection * bar_len, str_add_space_left(pad_str_left(mp[curr_selection].name, bar_len - 1)));
            set_color(wmain, MAINMENUCOLOR);

            old_selection = curr_selection;
            wrefresh(wmain);
        }

        switch (c = (key != ERR ? key : wait_for_key())) {
        case KEY_DOWN:
        case '\n':
            touchwin(wbody);
            wrefresh(wbody);
            remove_error_message();
            set_menu_position(TH + MH, curr_selection * bar_len);
            curs_set(1);
            (mp[curr_selection].func)();
            curs_set(0);

            switch (key) {
            case KEY_LEFT:
                curr_selection = (curr_selection + n_items - 1) % n_items;
                key = ERR; /// key = '\n';
                break;
            case KEY_RIGHT:
                curr_selection = (curr_selection + 1) % n_items;
                key = ERR; /// key = '\n';
                break;
            default:
                key = ERR;
            }

            repaint_main_menu(mp, bar_len);
            old_selection = -1;

            break;
        case KEY_LEFT:
            curr_selection = (curr_selection + n_items - 1) % n_items;
            break;
        case KEY_RIGHT:
            curr_selection = (curr_selection + 1) % n_items;
            break;
        case KEY_ESC:
            main_help();
            break;
        }
    }

    remove_error_message();
    touchwin(wbody);
    wrefresh(wbody);
}

static void clean() {
    if (in_curses) {
        delwin(wtitle);
        delwin(wmain);
        delwin(wbody);
        delwin(wstatus);
        curs_set(1);
        endwin();
        in_curses = FALSE;
    }
}

/*********************** EXTERNAL FUNCTIONS ***********************/
void finish() {
    quit = TRUE;
}

void title_message(char *title) {
    mvwaddstr(wtitle, 0, 2, pad_str_left(title, BW - 3));
    wrefresh(wtitle);
}

void status_message(char *message) {
    mvwaddstr(wstatus, 1, 2, pad_str_left(message, BW - 3));
    wrefresh(wstatus);
}

void error_message(char *message, int sound_flag) {
    if (sound_flag)
        beep();
    mvwaddstr(wstatus, 0, 2, pad_str_left(message, BW - 3));
    wrefresh(wstatus);
}

void remove_status_message() {
    remove_line(wstatus, 1);
}

void remove_error_message() {
    remove_line(wstatus, 0);
}

void clear_body() {
    werase(wbody);
    wmove(wbody, 0, 0);
    touchwin(wbody);
    wrefresh(wbody);
}

bool key_pressed() {
    ch = wgetch(wbody);
    return ch != ERR;
}

int get_key() {
    int c = ch;
    ch = ERR;

#ifdef ALT_X
    quit = (c == ALT_X);
#endif // ALT_ESC

    return c;
}

int wait_for_key() {
    do
        idle();
    while(!key_pressed());
    return get_key();
}

Key* select_key(List *list) {
    int curr_selection = 1, old_selection = -1;
    ListElement *curr = list->head, *old = NULL;
    bool stop = FALSE;
    int rewrite_old_flag = 1;

    if (!list->head) {
        error_message("No keys in the list!", 1);
        return NULL;
    }

    curs_set(0);
    clear_body();
    repaint_body_keys(list->head, BH - 1, 1, 2);

    key = ERR;
    while (!stop && !quit) {
        if (curr_selection != old_selection) {
            if (old_selection != -1 && rewrite_old_flag)
                mvwaddstr(wbody, (old_selection - 1) % (BH - 1) + 1, 2, convert_key_to_string((Key*)old->info));

            set_color(wbody, BODYREVCOLOR);
            mvwaddstr(wbody, (curr_selection - 1) % (BH - 1) + 1, 2, convert_key_to_string((Key*)curr->info));
            set_color(wbody, BODYCOLOR);

            old_selection = curr_selection;
            old = curr;
            wrefresh(wbody);
        }

        switch (key = (key != ERR ? key : wait_for_key())) {
        case KEY_UP:
            curr_selection--;
            if (!curr_selection) {
                curr_selection = list->length;
                curr = list->tail;
                clear_body();
                repaint_body_keys(find_kth_prev(curr, list->length % (BH - 1) - 1), BH - 1, 1, 2);
                rewrite_old_flag = 0;
            }
            else {
                curr = curr->prev;
                if (curr_selection % (BH - 1) == 0) {
                    clear_body();
                    repaint_body_keys(find_kth_prev(curr, BH - 2), BH - 1, 1, 2);
                    rewrite_old_flag = 0;
                }
                else
                    rewrite_old_flag = 1;
            }
            key = ERR;
            break;
        case KEY_DOWN:
            curr_selection++;
            if (curr_selection > list->length) {
                curr_selection -= list->length;
                curr = list->head;
                clear_body();
                repaint_body_keys(curr, BH - 1, 1, 2);
                rewrite_old_flag = 0;
            }
            else {
                curr = curr->next;
                if ((curr_selection - 1) % (BH - 1) == 0) {
                    clear_body();
                    repaint_body_keys(curr, BH - 1, 1, 2);
                    rewrite_old_flag = 0;
                }
                else
                    rewrite_old_flag = 1;
            }
            key = ERR;
            break;
        case KEY_ESC:
            curr = NULL;
        case '\n':
            stop = TRUE;
            key = ERR;
            break;
        default:
            key = ERR;
        }
    }

    clear_body();

    if (curr)
        return (Key*)curr->info;
    error_message("No key selected!", 1);
    return NULL;
}

void do_menu(Menu *mp) {
    int y, x;
    int n_items, bar_len;
    int menu_width, menu_height;
    int old_selection = -1, curr_selection = 0;
    bool stop = FALSE;
    WINDOW *wmenu;

    curs_set(0);
    get_menu_position(&y, &x);
    menu_dimensions(mp, &n_items, &bar_len);
    menu_height = n_items + 2;
    menu_width = bar_len + 2;
    wmenu = newwin(menu_height, menu_width, y, x);
    color_box(wmenu, SUBMENUCOLOR, 1);
    repaint_menu(wmenu, mp);

    key = ERR;
    while (!stop && !quit) {
        if (curr_selection != old_selection) {
            if (old_selection != -1)
                mvwaddstr(wmenu, old_selection + 1, 1, str_add_space_left(pad_str_left(mp[old_selection].name, bar_len - 1)));

            set_color(wmenu, SUBMENUREVCOLOR);
            mvwaddstr(wmenu, curr_selection + 1, 1, str_add_space_left(pad_str_left(mp[curr_selection].name, bar_len - 1)));
            set_color(wmenu, SUBMENUCOLOR);
            status_message(mp[curr_selection].description);

            old_selection = curr_selection;
            wrefresh(wmenu);
        }

        switch (key = (key != ERR ? key : wait_for_key())) {
        case '\n':
            ///touchwin(wbody);
            ///wrefresh(wbody);
            ///set_menu_position(y + 1, x + 1);
            set_menu_position(y + curr_selection + 1, x + menu_width);
            remove_error_message();

            key = ERR;
            curs_set(1);
            (mp[curr_selection].func)();
            curs_set(0);

            repaint_menu(wmenu, mp);

            old_selection = -1;
            break;
        case KEY_UP:
            curr_selection = (curr_selection + n_items - 1) % n_items;
            key = ERR;
            break;
        case KEY_DOWN:
            curr_selection = (curr_selection + 1) % n_items;
            key = ERR;
            break;
        case KEY_ESC:
        case KEY_LEFT:
        case KEY_RIGHT:
            if (key == KEY_ESC)
                key = ERR;
            stop = TRUE;
            break;
        default:
            key = ERR;
        }
    }

    remove_error_message();
    delwin(wmenu);
    touchwin(wbody);
    wrefresh(wbody);
}

void start_menu(Menu *mp, char *title) {
    initscr();
    init_colors();
    in_curses = TRUE;

    wtitle = subwin(stdscr, TH, BW, 0, 0);
    wmain = subwin(stdscr, MH, BW, TH, 0);
    wbody = subwin(stdscr, BH, BW, TH + MH, 0);
    wstatus = subwin(stdscr, SH, BW, TH + MH + BH, 0);

    color_box(wtitle, TITLECOLOR, 0);
    color_box(wmain, MAINMENUCOLOR, 0);
    color_box(wbody, BODYCOLOR, 0);
    color_box(wstatus, STATUSCOLOR, 0);

    if (title)
        title_message(title);

    cbreak();
    noecho();
    curs_set(0);
    nodelay(wbody, TRUE);
    halfdelay(10);
    keypad(wbody, TRUE);
    scrollok(wbody, TRUE);

    leaveok(stdscr, TRUE);
    leaveok(wtitle, TRUE);
    leaveok(wmain, TRUE);
    leaveok(wstatus, TRUE);

    main_menu(mp);
    clean();
}