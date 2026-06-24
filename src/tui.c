#include "tui.h"
#include <curses.h>
#include <locale.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static const int MIN_WIDTH = 80;
static const int MIN_HEIGHT = 24;
static const char TAB_NAMES[][16] = {
    "General",
    "Overclocking",
    "Thermal Control",
    "Device Info",
};
static constexpr size_t TAB_COUNT = (sizeof(TAB_NAMES) / sizeof(TAB_NAMES[0]));

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

static ShouldContinue handle_input(int input) {
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

void tui_init() {
  if (setlocale(LC_ALL, "") == nullptr) {
    (void)fprintf(stderr, "warning: couldn't set locale\n");
  }

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, true);
  curs_set(0);
}

bool tui_run() {
  if (!enforce_minimum_size()) {
    return false;
  }
  for (;;) {
    erase();
    refresh();
    auto input = getch();
    auto should_continue = handle_input(input);
    if (!should_continue) {
      return false;
    }
  }
}

void tui_shutdown() { endwin(); }
