#include "oc_controller.h"
#include "colors.h"
#include "gpu.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static constexpr size_t LABEL_SIZE = 32;

typedef struct {
  char label[LABEL_SIZE];
  int min;
  int max;
  int current;
} SliderState;

struct OCController {
  SliderState sliders[SLIDER_COUNT];
  Slider selected_slider;
  bool dirty;
};

static void init_slider(OCController *controller, const GpuState *gpu_state,
                        Slider slider) {
  auto state = &controller->sliders[slider];
#define INIT_LABEL(str) (void)snprintf(state->label, LABEL_SIZE, str)
  switch (slider) {
  case SLIDER_POWER_LIMIT:
    INIT_LABEL("Power Limit");
    state->min = (int)gpu_state->power_limit_min_milliwatts;
    state->max = (int)gpu_state->power_limit_max_milliwatts;
    state->current = (int)gpu_state->power_limit_milliwatts;
    break;
  case SLIDER_CORE_CLOCK_OFFSET:
    INIT_LABEL("Core Clock Offset");
    state->min = gpu_state->gpc_clock_offset_min_mhz;
    state->max = gpu_state->gpc_clock_offset_max_mhz;
    state->current = gpu_state->gpc_clock_offset_mhz;
    break;
  case SLIDER_MEM_CLOCK_OFFSET:
    INIT_LABEL("Mem Clock Offset");
    state->min = gpu_state->mem_clock_offset_min_mhz;
    state->max = gpu_state->mem_clock_offset_max_mhz;
    state->current = gpu_state->mem_clock_offset_mhz;
    break;
  case SLIDER_COUNT:
    break;
  }
#undef INIT_LABEL
}

OCController *oc_controller_create(const GpuState *gpu_state) {
  OCController *controller = calloc(1, sizeof(OCController));

  for (int i = 0; i < SLIDER_COUNT; ++i) {
    init_slider(controller, gpu_state, (Slider)i);
  }

  return controller;
}

void oc_controller_draw_slider(const OCController *controller, Slider slider,
                               WINDOW *window) {
  const SliderState *state = &controller->sliders[slider];
  int cols = getmaxx(window);

  if (slider == controller->selected_slider) {
    wattr_set(window, A_BOLD, PAIR_SELECTION, nullptr);
  }
  mvwprintw(window, 0, 0, "%s", state->label);
  wattr_set(window, A_NORMAL, 0, nullptr);

  int range = state->max - state->min;
  assert(range > 0);

  assert (state->current >= state->min && state->current <= state->max);
  int fill_width = (cols * (state->current - state->min)) / range;

  wmove(window, 1, 0);
  for (int i = 0; i < cols; ++i) {
    waddstr(window, i < fill_width ? "█" : "░");
  }

  mvwprintw(window, 2, 0, "%d", state->min);
  int max_len = snprintf(nullptr, 0, "%d", state->max);
  mvwprintw(window, 2, cols - max_len, "%d", state->max);
}

void oc_controller_destroy(OCController *controller) { free(controller); }
