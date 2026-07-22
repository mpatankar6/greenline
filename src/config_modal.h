#pragma once
#include "data_source.h"
#include <ncurses.h>

typedef struct PlotController PlotController; // Avoid circular dep

typedef DataSource (*SelectionFn)(const PlotController *);
typedef struct {
  SelectionFn get_left;
  SelectionFn get_right;
} CurrentSelectionGetters;
typedef struct ConfigModal ConfigModal;

[[nodiscard]]
ConfigModal *config_modal_create(PlotController *plot_controller,
                                 CurrentSelectionGetters getters);

void config_modal_draw(const ConfigModal *config_modal, WINDOW *window);

void config_modal_handle_input(ConfigModal *config_modal, int key);

void config_modal_destroy(ConfigModal *config_modal);
