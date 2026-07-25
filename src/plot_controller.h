#pragma once
#include "config_modal.h"
#include "data_source.h"
#include "gpu.h"
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
                                    PlotProfile new_profile);

void plot_controller_feed_data(const PlotController *plot_controller,
                               const GpuState *gpu_state);

void plot_controller_draw(const PlotController *plot_controller,
                          const GpuState *gpu_state, WINDOW *window);

[[nodiscard]]
ConfigModal *plot_controller_get_config_modal(PlotController *plot_controller);

void plot_controller_update_selection(PlotController *plot_controller,
                                      ModalSide side, DataSource new_source);

void plot_controller_destroy(PlotController *plot_controller);
