#include "tui_plot.h"
#include "circular_buffer.h"
#include "gpu.h"
#include <assert.h>
#include <curses.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define OFFSET_AND_TYPE(field)                                                 \
  offsetof(GpuState, field), _Generic((typeof((GpuState){0}.field)){0},        \
      unsigned int: DS_UINT,                                                   \
      double: DS_DOUBLE)

typedef enum { DS_UINT, DS_DOUBLE } DSType;

typedef struct DataSource {
  char name[32];
  char unit[4];
  int lower;
  int upper;
  size_t offset;
  DSType type;
} DataSource;

static constexpr DataSource SOURCES[] = {
    {"GPU Clock", "Mhz", 0, 100, OFFSET_AND_TYPE(gpu_util_percent)},
    {"GPU Temp", "°C", 0, 95, OFFSET_AND_TYPE(temperature_celsius)}};
static constexpr size_t SOURCE_COUNT = sizeof(SOURCES) / sizeof(DataSource);

static constexpr int NUM_TICK_LABELS = 5;

struct Plot {
  CircularBuffer *data;
  GpuState *gpu_state;
  Plot *paired;
  DataSource data_source;
};

static void draw_data_point(int x_coord, int y_coord, int last_y_coord,
                            WINDOW *window) {
#define FLIP(y_coord) (getmaxy(window) - 1 - (y_coord))
  if (last_y_coord == -1) {
    mvwaddch(window, FLIP(y_coord), x_coord, ACS_HLINE);
    return;
  }
  int height_difference = y_coord - last_y_coord;
  int height_distance = abs(height_difference);
  if (height_difference > 0) { // Draw down
    // mvwvline(window, FLIP(y_coord), x_coord, height_distance, ACS_VLINE);
    mvwvline(window, FLIP(y_coord), x_coord, ACS_VLINE, height_distance);
    mvwaddch(window, FLIP(y_coord), x_coord, ACS_URCORNER);
    mvwaddch(window, FLIP(last_y_coord), x_coord, ACS_LLCORNER);
  } else if (height_difference < 0) { // Draw up
    mvwvline(window, FLIP(last_y_coord), x_coord, ACS_VLINE, height_distance);
    mvwaddch(window, FLIP(y_coord), x_coord, ACS_LRCORNER);
    mvwaddch(window, FLIP(last_y_coord), x_coord, ACS_ULCORNER);
  } else { // Draw straight
    mvwaddch(window, FLIP(y_coord), x_coord, ACS_HLINE);
  }
#undef FLIP
}

static void draw_data_line(Plot *plot, WINDOW *window) {
  wattr_set(window, A_BOLD, 1, nullptr);
  int rows = getmaxy(window);
  int cols = getmaxx(window) - 2; // Account for side borders
  size_t buffer_size = circular_buffer_size(plot->data);
  // Draw as much as we can, ie min(number of columns, number of data points)
  size_t points_to_draw =
      (size_t)cols < buffer_size ? (size_t)cols : buffer_size;
  // Clamp right edge to the last column
  int x_pos = (int)points_to_draw < cols ? (int)points_to_draw : cols;
  int last_height = -1; // Sentinel value used to indicate first height
  int data_point_range = plot->data_source.upper - plot->data_source.lower;
  assert(data_point_range > 0);
  for (size_t i = 0; i < points_to_draw; ++i, --x_pos) {
    int data_point = circular_buffer_peek(plot->data, i);
    // Y-pos assumes bottom-left origin, can be flipped later.
    int y_pos = (int)roundl((double)(rows - 1) * data_point / data_point_range);
    y_pos = y_pos >= rows ? rows - 1 : y_pos; // Clamp y_pos

    draw_data_point(x_pos, y_pos, last_height, window);
    last_height = y_pos;
  }
  wattr_set(window, A_NORMAL, 0, nullptr);
}

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

  // Draw plot border and title
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
  draw_data_line(plot, data_pane);
  delwin(data_pane);
}

Plot *plot_create() {
  Plot *plot = calloc(1, sizeof(Plot));
  plot->data = circular_buffer_create();
  plot->data_source = SOURCES[0];
  return plot;
}

void plot_update(Plot *plot, const GpuState *state) {
  auto value_address = (char *)state + plot->data_source.offset;
  switch (plot->data_source.type) {
  case DS_UINT:
    circular_buffer_put(plot->data, (int)*(unsigned int *)value_address);
    break;
  case DS_DOUBLE:
    circular_buffer_put(plot->data, (int)*(double *)value_address);
    break;
  }
}

void plot_configure(Plot* plot, WINDOW* window) {
  werase(window);
  box(window, 0, 0);
}

void plot_configure_model_handle_input(Plot *plot, int key) {
}

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
