#include "gpu.h"
#include <curses.h>

typedef struct Plot Plot;

Plot *plot_create();

void plot_configure(Plot* plot, WINDOW *window);

void plot_configure_model_handle_input(Plot* plot, int key);

void plot_update(Plot* plot, const GpuState *state);

void plot_draw(Plot* plot, WINDOW *window);

void plot_destroy(Plot *plot);
