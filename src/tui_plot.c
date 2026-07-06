#include "tui_plot.h"
#include "circular_buffer.h"
#include <assert.h>
#include <curses.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct DataSource {
  char name[32];
  char unit[4];
  int lower;
  int upper;
  void *value;
} DataSource;

static constexpr DataSource SOURCES[] = {{"GPU Clock", "Mhz", 0, 100, nullptr}};
static constexpr size_t SOURCE_COUNT = sizeof(SOURCES) / sizeof(DataSource);

static constexpr int NUM_TICK_LABELS = 5;

struct Plot {
  CircularBuffer *data;
  Plot *paired;
  DataSource data_source;
};

static void plot_draw_pane(Plot *plot, WINDOW *window) {
  int cols = getmaxx(window);
  int rows = getmaxy(window);
  auto data_source = plot->data_source;

  // Write tick labels bottom to top
  int longest_tick_label_len = 0;
  int row_step = (rows - 2) / (NUM_TICK_LABELS - 1);
  int data_step =
      (data_source.upper - data_source.lower) / (NUM_TICK_LABELS - 1);
  int row = rows - 1;
  int value = data_source.lower;
  for (size_t i = 0; i < NUM_TICK_LABELS; ++i) {
    char buffer[16];
    auto tick_label_len = snprintf(buffer, sizeof(buffer), "%d", value);
    longest_tick_label_len = tick_label_len > longest_tick_label_len
                                 ? tick_label_len
                                 : longest_tick_label_len;
    mvwprintw(window, row, 0, "%s", buffer);
    row -= row_step;
    value += data_step;
  }

  int x_offset = longest_tick_label_len;
  int plot_cols = cols - x_offset;
  int plot_rows = (row_step * (NUM_TICK_LABELS - 1)) + 1;
  char title_buffer[64];
  int title_len = snprintf(title_buffer, sizeof(title_buffer), "%s (%s)",
                           data_source.name, data_source.unit);
  mvwprintw(window, rows - plot_rows - 1, (plot_cols - title_len) / 2, "%s",
            title_buffer);
  auto data_pane =
      derwin(window, plot_rows, plot_cols, rows - plot_rows, x_offset);
  box(data_pane, 0, 0);
  delwin(data_pane);
}

Plot *plot_create() {
  Plot *plot = calloc(1, sizeof(Plot));
  plot->data = circular_buffer_create();
  plot->data_source = SOURCES[0];
  return plot;
}

Plot *plot_configure() {}

void plot_draw(Plot *plot, WINDOW *window) {
  int cols = getmaxx(window);
  int rows = getmaxy(window);
  if (plot->paired != nullptr) {
    auto left_pane = derwin(window, rows, cols / 2, 0, 0);
    auto right_pane = derwin(window, rows, cols / 2, 0, cols / 2);
    plot_draw_pane(plot, left_pane);
    plot_draw_pane(plot->paired, right_pane);
    delwin(left_pane);
    delwin(right_pane);
  } else {
    plot_draw_pane(plot, window);
  }
}

void plot_destroy(Plot *plot) {
  if (plot->paired != nullptr) {
    circular_buffer_destroy(plot->paired->data);
    free(plot->paired);
  }
  circular_buffer_destroy(plot->data);
  free(plot);
}
