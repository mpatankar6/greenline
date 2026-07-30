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
  bool dirty;
} SliderState;

struct OCController {
  SliderState sliders[SLIDER_COUNT];
  Slider selected_slider;
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
  static constexpr int DISPLAY_DIVISOR[SLIDER_COUNT] = {
      [SLIDER_POWER_LIMIT] = 1000,
      [SLIDER_CORE_CLOCK_OFFSET] = 1,
      [SLIDER_MEM_CLOCK_OFFSET] = 1,
  };
  static const char *const UNIT[SLIDER_COUNT] = {
      [SLIDER_POWER_LIMIT] = "W",
      [SLIDER_CORE_CLOCK_OFFSET] = "MHz",
      [SLIDER_MEM_CLOCK_OFFSET] = "MHz",
  };

  const SliderState *state = &controller->sliders[slider];
  int cols = getmaxx(window);
  int divisor = DISPLAY_DIVISOR[slider];
  const char *unit = UNIT[slider];

  mvwprintw(window, 0, 0, "%s", state->label);

  bool selected = slider == controller->selected_slider;
  int display_current = state->current / divisor;
  int current_len = snprintf(nullptr, 0, "%d %s", display_current, unit);
  mvwprintw(window, 0, cols - current_len, "%d %s", display_current, unit);

  int range = state->max - state->min;
  assert(range > 0);

  assert(state->current >= state->min && state->current <= state->max);
  int fill_width = (cols * (state->current - state->min)) / range;

  if (selected) {
    wattr_set(window, A_NORMAL, PAIR_SELECTION, nullptr);
  }
  wmove(window, 1, 0);
  for (int i = 0; i < cols; ++i) {
    waddstr(window, i < fill_width ? "█" : "░");
  }
  wattr_set(window, A_NORMAL, 0, nullptr);

  int display_min = state->min / divisor;
  int display_max = state->max / divisor;
  mvwprintw(window, 2, 0, "%d %s", display_min, unit);
  int max_len = snprintf(nullptr, 0, "%d %s", display_max, unit);
  mvwprintw(window, 2, cols - max_len, "%d %s", display_max, unit);
}

// TODO implement gpu sets
static void apply_slider(SliderState *state) { state->dirty = false; }

// Abandons the pending edit and reverts to whatever is live on the GPU
static void discard_slider(SliderState *state, const GpuState *gpu_state,
                           Slider slider) {
  switch (slider) {
  case SLIDER_POWER_LIMIT:
    state->current = (int)gpu_state->power_limit_milliwatts;
    break;
  case SLIDER_CORE_CLOCK_OFFSET:
    state->current = gpu_state->gpc_clock_offset_mhz;
    break;
  case SLIDER_MEM_CLOCK_OFFSET:
    state->current = gpu_state->mem_clock_offset_mhz;
    break;
  case SLIDER_COUNT:
    break;
  }
  state->dirty = false;
}

// Abandons the pending edit and resets to factory-default value
static void reset_slider(SliderState *state, const GpuState *gpu_state,
                         Slider slider) {
  switch (slider) {
  case SLIDER_POWER_LIMIT:
    state->current = (int)gpu_state->tdp_milliwatts;
    break;
  case SLIDER_CORE_CLOCK_OFFSET:
  case SLIDER_MEM_CLOCK_OFFSET:
    state->current = 0;
    break;
  case SLIDER_COUNT:
    break;
  }
  state->dirty = true;
}

static void nudge_slider(SliderState *state, Slider slider, bool forward) {
  static constexpr int STEPS[SLIDER_COUNT] = {
      [SLIDER_POWER_LIMIT] = 5000,
      [SLIDER_CORE_CLOCK_OFFSET] = 5,
      [SLIDER_MEM_CLOCK_OFFSET] = 10,
  };
  int step = STEPS[slider];

  int next = state->current;
  if (forward) {
    next += step;
  } else {
    next -= step;
  }

  // Clamp to slider's bounds
  if (next < state->min) {
    next = state->min;
  } else if (next > state->max) {
    next = state->max;
  }

  state->current = next;
  state->dirty = true;
}

void oc_controller_handle_input(OCController *controller,
                                const GpuState *gpu_state, int key) {
  SliderState *selected = &controller->sliders[controller->selected_slider];
  switch (key) {
  case KEY_DOWN:
  case 'j':
    if (controller->selected_slider < SLIDER_COUNT - 1) {
      controller->selected_slider = (Slider)(controller->selected_slider + 1);
    }
    break;
  case KEY_UP:
  case 'k':
    if (controller->selected_slider > 0) {
      controller->selected_slider = (Slider)(controller->selected_slider - 1);
    }
    break;
  case KEY_LEFT:
  case 'h':
    nudge_slider(selected, controller->selected_slider, false);
    break;
  case KEY_RIGHT:
  case 'l':
    nudge_slider(selected, controller->selected_slider, true);
    break;
  case 'd':
    discard_slider(selected, gpu_state, controller->selected_slider);
    break;
  case 'r':
    for (int i = 0; i < SLIDER_COUNT; ++i) {
      reset_slider(&controller->sliders[i], gpu_state, (Slider)i);
      apply_slider(&controller->sliders[i]);
    }
    break;
  case 'a':
    apply_slider(selected);
    break;
  default:
    return;
  }
}

void oc_controller_destroy(OCController *controller) { free(controller); }
