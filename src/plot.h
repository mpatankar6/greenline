#pragma once
#include "data_source.h"
#include "gpu.h"
#include <ncurses.h>

typedef struct Plot Plot;

Plot *plot_create(DataSource data_source);

DataSource plot_data_source(const Plot *plot);

void plot_feed_data(Plot *plot, const GpuState *gpu_state);

void plot_draw(const Plot *plot, const GpuState *gpu_state, WINDOW *window);

void plot_destroy(Plot *plot);
