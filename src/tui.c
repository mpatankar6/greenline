#include "tui.h"
#include <assert.h>
#include <curses.h>
#include <locale.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char TITLE[] = "Greenline";
static const int MIN_WIDTH = 80;
static const int MIN_HEIGHT = 24;

enum {
  PAIR_TITLE = 1,
  PAIR_SELECTED_TAB,
};
static void init_colors() {
  start_color();
  use_default_colors();
  init_pair(PAIR_TITLE, COLOR_GREEN, -1);
  init_pair(PAIR_SELECTED_TAB, COLOR_BLUE, -1);
}

typedef struct Tab {
  const char *name;
} Tab;
static const Tab TABS[] = {
    {"General"},
    {"OC"},
    {"Thermals"},
    {"Info"},
};
static constexpr size_t TAB_COUNT = (sizeof(TABS) / sizeof(TABS[0]));
static size_t selected_tab_index = 1;

typedef bool ShouldContinue;

static ShouldContinue enforce_minimum_size() {
  while (COLS < MIN_WIDTH || LINES < MIN_HEIGHT) {
    erase();
    auto error_text = "Terminal too small, should be at least 80 x 24";
    auto y_pos = LINES / 2;
    auto x_pos = (COLS - (int)strlen(error_text)) / 2;
    x_pos = x_pos < 0 ? 0 : x_pos; // Clamp x to be positive
    mvprintw(y_pos, x_pos, "%s", error_text);
    refresh();
    auto input = getch();
    if (input == KEY_RESIZE) {
      continue;
    }
    if (input == 'q') {
      return false;
    }
  }

  return true;
}

static ShouldContinue handle_input() {
  auto input = getch();
  if (input >= '1' && input <= '4') {
    selected_tab_index = input - '0';
    return true;
  }
  switch (input) {
  case KEY_RESIZE:
    return enforce_minimum_size();
  case 'q':
    return false;
  default:
    // Unrecognized key
    break;
  }

  return true;
}

static void draw_tab_line() {
  int x_pos = 2;
  for (size_t tab_index = 1; tab_index <= TAB_COUNT; ++tab_index) {
    auto tab = TABS[tab_index - 1];
    char name[256];
    (void)snprintf(name, sizeof(name), " [%zu] %s ", tab_index, tab.name);
    if (tab_index == selected_tab_index) {
      attr_set(A_BOLD, PAIR_SELECTED_TAB, nullptr);
    }
    mvprintw(0, x_pos, "%s", name);
    x_pos += (int)strlen(name);
    attr_set(A_NORMAL, 0, nullptr);
  }
}

static void draw_title() {
  int x_pos = COLS - (int)strlen(TITLE) - 4;
  attr_set(A_BOLD, PAIR_TITLE, nullptr);
  mvprintw(0, x_pos, " %s ", TITLE);
  attr_set(A_NORMAL, 0, nullptr);
}

static void draw_frame() {
  mvhline(0, 1, ACS_HLINE, COLS - 2);
  mvhline(LINES - 1, 1, ACS_HLINE, COLS - 2);
  mvvline(1, 0, ACS_VLINE, LINES - 2);
  mvvline(1, COLS - 1, ACS_VLINE, LINES - 2);
  mvaddch(0, 0, ACS_ULCORNER);
  mvaddch(0, COLS - 1, ACS_URCORNER);
  mvaddch(LINES - 1, 0, ACS_LLCORNER);
  mvaddch(LINES - 1, COLS - 1, ACS_LRCORNER);
  draw_tab_line();
  draw_title();
}

void tui_init() {
  if (setlocale(LC_ALL, "") == nullptr) {
    (void)fprintf(stderr, "warning: couldn't set locale\n");
  }

  initscr();
  init_colors();
  cbreak();
  noecho();
  keypad(stdscr, true);
  curs_set(0);
}

void tui_run() {
  if (!enforce_minimum_size()) {
    return;
  }
  for (;;) {
    erase();
    draw_frame();
    refresh();
    auto should_continue = handle_input();
    if (!should_continue) {
      return;
    }
  }
}

void tui_shutdown() { endwin(); }
