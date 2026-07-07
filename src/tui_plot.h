#include "gpu.h"
#include <curses.h>

typedef struct Plot Plot;

Plot *plot_create();

void plot_update(Plot* plot, const GpuState *state);

void plot_draw(Plot* plot, WINDOW *window);

void plot_destroy(Plot *plot);
