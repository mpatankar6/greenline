#include "plot_controller.h"
#include "config_modal.h"
#include "data_source.h"
#include "gpu.h"
#include "plot.h"
#include <assert.h>
#include <stdlib.h>

typedef struct PairedPlot {
  Plot *left_subplot;
  Plot *right_subplot;
} PairedPlot;

struct PlotController {
  PairedPlot plots[PLOT_PROFILE_COUNT];
  PlotProfile current_profile;
  ConfigModal *config_modal;
};

static DataSource get_left_selection(const PlotController *controller) {
  auto subplot = controller->plots[controller->current_profile].left_subplot;
  return subplot == nullptr ? NULL_SOURCE : plot_data_source(subplot);
}

static DataSource get_right_selection(const PlotController *controller) {
  auto subplot = controller->plots[controller->current_profile].right_subplot;
  return subplot == nullptr ? NULL_SOURCE : plot_data_source(subplot);
}

static Plot **get_subplot_slot(PlotController *controller, ModalSide side) {
  auto plot = &controller->plots[controller->current_profile];
  switch (side) {
  case MODAL_LEFT:
    return &plot->left_subplot;
  case MODAL_RIGHT:
    return &plot->right_subplot;
  }
}

PlotController *plot_controller_create() {
  PlotController *plot_controller = calloc(1, sizeof(PlotController));

  // Initialize default plots
  plot_controller->plots[PLOT_PROFILE_GENERAL].left_subplot =
      plot_create(SOURCES[0]);
  plot_controller->plots[PLOT_PROFILE_OC].left_subplot =
      plot_create(SOURCES[0]);
  plot_controller->plots[PLOT_PROFILE_OC].right_subplot =
      plot_create(SOURCES[3]);
  plot_controller->plots[PLOT_PROFILE_THERMALS].left_subplot =
      plot_create(SOURCES[3]);
  plot_controller->plots[PLOT_PROFILE_THERMALS].right_subplot =
      plot_create(SOURCES[4]);

  CurrentSelectionGetters getters = {.get_left = get_left_selection,
                                     .get_right = get_right_selection};
  plot_controller->config_modal = config_modal_create(plot_controller, getters);

  return plot_controller;
}

ConfigModal *plot_controller_get_config_modal(PlotController *plot_controller) {
  return plot_controller->config_modal;
}

void plot_controller_switch_profile(PlotController *plot_controller,
                                    PlotProfile new_profile) {
  plot_controller->current_profile = new_profile;
}

void plot_controller_feed_data(const PlotController *plot_controller,
                               const GpuState *gpu_state) {
  auto plot_pair = plot_controller->plots[plot_controller->current_profile];
  if (plot_pair.left_subplot != nullptr) {
    plot_feed_data(plot_pair.left_subplot, gpu_state);
  }
  if (plot_pair.right_subplot != nullptr) {
    plot_feed_data(plot_pair.right_subplot, gpu_state);
  }
}

void plot_controller_draw(const PlotController *plot_controller,
                          const GpuState *gpu_state, WINDOW *window) {
  auto plot_pair = plot_controller->plots[plot_controller->current_profile];
  assert(plot_pair.left_subplot != nullptr); // We at least need a left plot

  int cols = getmaxx(window);
  int rows = getmaxy(window);
  if (plot_pair.right_subplot != nullptr) {
    auto left_pane = derwin(window, rows, cols / 2, 0, 0);
    auto right_pane = derwin(window, rows, cols / 2, 0, cols / 2);
    plot_draw(plot_pair.left_subplot, gpu_state, left_pane);
    plot_draw(plot_pair.right_subplot, gpu_state, right_pane);
    delwin(left_pane);
    delwin(right_pane);
  } else {
    plot_draw(plot_pair.left_subplot, gpu_state, window);
    delwin(window);
  }
}

void plot_controller_update_selection(PlotController *plot_controller,
                                      ModalSide side, DataSource new_source) {
  auto subplot_slot = get_subplot_slot(plot_controller, side);
  auto subplot = *subplot_slot;

  if (subplot == nullptr) {
    assert(!sources_equal(new_source, NULL_SOURCE));
    *subplot_slot = plot_create(new_source);
    return;
  }

  if (!sources_equal(new_source, plot_data_source(subplot))) {
    plot_destroy(subplot);
    if (sources_equal(new_source, NULL_SOURCE)) {
      *subplot_slot = nullptr;
    } else {
      *subplot_slot = plot_create(new_source);
    }
  }
}

void plot_controller_destroy(PlotController *plot_controller) {
  config_modal_destroy(plot_controller->config_modal);
  for (int i =0; i < PLOT_PROFILE_COUNT ; ++i ) {
    auto plot_pair = plot_controller->plots[i];
    plot_destroy(plot_pair.left_subplot);
    plot_destroy(plot_pair.right_subplot);
  }
  free(plot_controller);
}
