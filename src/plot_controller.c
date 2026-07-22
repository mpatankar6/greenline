#include "plot_controller.h"
#include "circular_buffer.h"
#include "config_modal.h"
#include "data_source.h"
#include "plot.h"
#include <stdlib.h>

typedef struct Plot {
  CircularBuffer *data;
  DataSource data_source;
} Plot;

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
  return subplot == nullptr ? NULL_SOURCE : subplot->data_source;
}

static DataSource get_right_selection(const PlotController *controller) {
  auto subplot = controller->plots[controller->current_profile].right_subplot;
  return subplot == nullptr ? NULL_SOURCE : subplot->data_source;
}

PlotController *plot_controller_create() {
  PlotController *plot_controller = calloc(1, sizeof(PlotController));

  // Initialize default plots
  Plot *general_left = calloc(1, sizeof(Plot));
  general_left->data_source = SOURCES[0];
  general_left->data = circular_buffer_create();
  plot_controller->plots[PLOT_PROFILE_GENERAL].left_subplot = general_left;

  CurrentSelectionGetters getters = {.get_left = get_left_selection,
                                     .get_right = get_right_selection};
  plot_controller->config_modal = config_modal_create(plot_controller, getters);

  return plot_controller;
}

ConfigModal *plot_controller_get_config_modal(PlotController *plot_controller) {
  return plot_controller->config_modal;
}

void plot_controller_destroy(PlotController *plot_controller) {
  config_modal_destroy(plot_controller->config_modal);
  free(plot_controller);
}
