#pragma once
#include "config_modal.h"
#include <ncurses.h>

typedef struct PlotController PlotController;
typedef enum PlotProfile {
  PLOT_PROFILE_GENERAL,
  PLOT_PROFILE_OC,
  PLOT_PROFILE_THERMALS,

  PLOT_PROFILE_COUNT
} PlotProfile;

[[nodiscard]]
PlotController *plot_controller_create();

void plot_controller_switch_profile(PlotController *plot_controller,
                                    PlotProfile active_plot);

void plot_controller_draw_plot(const PlotController *plot_controller,
                               WINDOW *window);

[[nodiscard]]
ConfigModal* plot_controller_get_config_modal(PlotController *plot_controller);

void plot_controller_destroy(PlotController *plot_controller);
