#pragma once
#include "gpu.h"
#include <ncurses.h>

typedef enum {
  SLIDER_POWER_LIMIT,
  SLIDER_CORE_CLOCK_OFFSET,
  SLIDER_MEM_CLOCK_OFFSET,

  SLIDER_COUNT
} Slider;

typedef struct OCController OCController;

[[nodiscard]]
OCController *oc_controller_create(const GpuState *gpu_state);

void oc_controller_draw_slider(const OCController *controller, Slider slider,
                               WINDOW *window);

void oc_controller_handle_input(OCController *controller,
                                const GpuState *gpu_state, int key);

void oc_controller_destroy(OCController *controller);
