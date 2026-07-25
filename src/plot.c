#include "plot.h"
#include "circular_buffer.h"
#include "colors.h"
#include "data_source.h"
#include "gpu.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Plot {
  CircularBuffer *data;
  DataSource data_source;
} Plot;

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

static void draw_data_line(const Plot *plot, WINDOW *window,
                           int data_point_range) {
  wattr_set(window, A_BOLD, PAIR_PLOT_LINE, nullptr);
  int rows = getmaxy(window);
  int cols = getmaxx(window) - 2; // Account for side borders
  size_t buffer_size = circular_buffer_size(plot->data);
  // Draw as much as we can, ie min(number of columns, number of data points)
  size_t points_to_draw =
      (size_t)cols < buffer_size ? (size_t)cols : buffer_size;
  // Clamp right edge to the last column
  int x_pos = (int)points_to_draw < cols ? (int)points_to_draw : cols;
  int last_height = -1; // Sentinel value used to indicate first height
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

Plot *plot_create(DataSource data_source) {
  Plot *plot = calloc(1, sizeof(Plot));
  plot->data = circular_buffer_create();
  plot->data_source = data_source;
  return plot;
}

DataSource plot_data_source(const Plot *plot) { return plot->data_source; }

void plot_feed_data(Plot *plot, const GpuState *gpu_state) {
  auto value = data_source_value(plot->data_source, gpu_state);
  circular_buffer_put(plot->data, value);
}

void plot_draw(const Plot *plot, const GpuState *gpu_state, WINDOW *window) {
  static constexpr int NUM_TICK_LABELS = 5;

  int cols = getmaxx(window);
  int rows = getmaxy(window);
  auto data_source = plot->data_source;

  auto lower_bound = data_source_lower(data_source, gpu_state);
  auto upper_bound = data_source_upper(data_source, gpu_state);
  auto data_point_range = upper_bound - lower_bound;
  assert(data_point_range > 0);

  // Write tick labels bottom to top
  int longest_tick_label_len = 0;
  int row_step = (rows - 2) / (NUM_TICK_LABELS - 1);
  int data_step = data_point_range / (NUM_TICK_LABELS - 1);
  int row = rows - 1;
  int value = lower_bound;
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
  draw_data_line(plot, data_pane, data_point_range);
  delwin(data_pane);
}

void plot_destroy(Plot *plot) {
  if (plot == nullptr) {
    return;
  }
  circular_buffer_destroy(plot->data);
  free(plot);
}
