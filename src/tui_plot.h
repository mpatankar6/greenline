#include <curses.h>

typedef struct Plot Plot;

Plot *plot_create();

void plot_draw(Plot* plot, WINDOW *window);

void plot_destroy(Plot *plot);
