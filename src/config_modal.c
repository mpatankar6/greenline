#include "config_modal.h"
#include "colors.h"
#include "data_source.h"
#include "plot_controller.h"
#include <assert.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ConfigModal {
  int current_row;
  int current_col;
  PlotController *plot_controller;
  SelectionFn get_left_selection;
  SelectionFn get_right_selection;
  size_t longest_source_name;
};

static DataSource get_selected_source(const ConfigModal *modal,
                                      ModalSide side) {
  switch (side) {
  case MODAL_LEFT:
    return modal->get_left_selection(modal->plot_controller);
  case MODAL_RIGHT:
    return modal->get_right_selection(modal->plot_controller);
  }
  unreachable();
}

static void draw_source_list(const ConfigModal *config_modal, WINDOW *window,
                             ModalSide side) {
  int longest_text_width =
      (int)(config_modal->longest_source_name + strlen("[ ] "));
  int x_pos = ((getmaxx(window) - longest_text_width) / 2);

  const char *label = side == MODAL_LEFT ? "Plot 1" : "Plot 2";
  int label_x_pos = (getmaxx(window) - (int)strlen(label)) / 2;
  int y_pos = 0;
  wattr_set(window, A_BOLD, PAIR_HEADING, nullptr);
  mvwprintw(window, y_pos++, label_x_pos, "%s", label);
  wattr_set(window, A_NORMAL, 0, nullptr);

  for (size_t i = 0; i < SOURCE_COUNT; ++i) {
    auto selected_source = get_selected_source(config_modal, side);
    char checkbox[] = "[ ]";
    if (sources_equal(SOURCES[i], selected_source)) {
      strcpy(checkbox, "[*]");
    }
    if (config_modal->current_col == (int)side &&
        config_modal->current_row == (int)i) {
      wattr_set(window, A_REVERSE, PAIR_SELECTION, nullptr);
    }
    mvwprintw(window, y_pos++, x_pos, "%s %-*s", checkbox,
              (int)config_modal->longest_source_name, SOURCES[i].name);
    wattr_set(window, A_NORMAL, 0, nullptr);
  }
}

ConfigModal *config_modal_create(PlotController *plot_controller,
                                 CurrentSelectionGetters getters) {
  auto modal = (ConfigModal *)calloc(1, sizeof(ConfigModal));
  modal->plot_controller = plot_controller;
  modal->get_left_selection = getters.get_left;
  modal->get_right_selection = getters.get_right;

  for (size_t i = 0; i < SOURCE_COUNT; ++i) {
    size_t len = strlen(SOURCES[i].name);
    if (len > modal->longest_source_name) {
      modal->longest_source_name = len;
    }
  }

  return modal;
}

void config_modal_draw(const ConfigModal *config_modal, WINDOW *window) {
  static constexpr char MODAL_TITLE[] = "Configure Plots";
  werase(window);
  box(window, 0, 0);
  int x_pos = (getmaxx(window) - (int)strlen(MODAL_TITLE)) / 2;
  mvwprintw(window, 1, x_pos, MODAL_TITLE);

  static constexpr int VERTICAL_PADDING = 3;
  int width = getmaxx(window) / 2;
  int height = getmaxy(window) - (2 * VERTICAL_PADDING);
  auto left_side = derwin(window, height, width, VERTICAL_PADDING, 1);
  auto right_side = derwin(window, height, width, VERTICAL_PADDING, width);
  draw_source_list(config_modal, left_side, MODAL_LEFT);
  draw_source_list(config_modal, right_side, MODAL_RIGHT);
  delwin(left_side);
  delwin(right_side);
}

void config_modal_handle_input(ConfigModal *config_modal, int key) {
  int *row = &config_modal->current_row;
  int *col = &config_modal->current_col;
  switch (key) {
  case KEY_LEFT:
  case 'h':
    *col = 0;
    break;
  case KEY_DOWN:
  case 'j':
    *row = *row < (int)SOURCE_COUNT - 2 ? *row + 1 : *row;
    break;
  case KEY_UP:
  case 'k':
    *row = *row > 0 ? *row - 1 : *row;
    break;
  case KEY_RIGHT:
  case 'l':
    *col = 1;
    break;
  case ' ':
    auto side = (ModalSide)*col;
    auto selection = SOURCES[*row];
    if (side != MODAL_LEFT && // The left plot should exist at minimum
        sources_equal(selection, get_selected_source(config_modal, side))) {
      selection = NULL_SOURCE;
    }
    plot_controller_update_selection(config_modal->plot_controller, side,
                                     selection);
    break;
  default:
    return;
  }
}

void config_modal_destroy(ConfigModal *config_modal) { free(config_modal); }
