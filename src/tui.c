#include "tui.h"
#include "colors.h"
#include "config_modal.h"
#include "gpu.h"
#include "oc_controller.h"
#include "plot_controller.h"
#include <assert.h>
#include <curses.h>
#include <locale.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static constexpr char TITLE[] = "Greenline";
static constexpr int MIN_WIDTH = 80;
static constexpr int MIN_HEIGHT = 24;

static unsigned int poll_ms = 2000;
static bool modal_active = false;
static bool is_root;

static void init_colors() {
  start_color();
  use_default_colors();
  init_pair(PAIR_TITLE, COLOR_GREEN, -1);
  init_pair(PAIR_SELECTION, COLOR_BLUE, -1);
  init_pair(PAIR_HEADING, COLOR_YELLOW, -1);
  init_pair(PAIR_PLOT_LINE, COLOR_CYAN, -1);
  init_pair(PAIR_DANGER, COLOR_RED, -1);
  init_pair(PAIR_SAFE, COLOR_GREEN, -1);
}

enum SelectedTab { TAB_GENERAL = 1, TAB_OC, TAB_THERMALS, TAB_INFO };
static enum SelectedTab selected_tab = TAB_GENERAL;

typedef bool ShouldContinue;

static ShouldContinue enforce_minimum_size() {
  while (COLS < MIN_WIDTH || LINES < MIN_HEIGHT) {
    erase();
    auto error_text = "Terminal too small, should be at least 80 x 24";
    auto y_pos = LINES / 2;
    auto x_pos = (COLS - (int)strlen(error_text)) / 2;
    x_pos = x_pos < 0 ? 0 : x_pos; // Clamp x to be positive
    mvprintw(y_pos, x_pos, "%s", error_text);
    refresh();
    auto input = getch();
    if (input == KEY_RESIZE) {
      continue;
    }
    if (input == 'q') {
      return false;
    }
  }

  return true;
}

static ShouldContinue handle_input(int input) {
  if (input >= '1' && input <= '4') {
    selected_tab = (enum SelectedTab)(input - '0');
    modal_active = false;
    return true;
  }
  switch (input) {
  case KEY_RESIZE:
    return enforce_minimum_size();
  case '=':
  case '+':
    poll_ms = poll_ms < 5000 ? poll_ms + 100 : poll_ms;
    break;
  case '-':
    poll_ms = poll_ms > 100 ? poll_ms - 100 : poll_ms;
    break;
  case 'c':
    if (selected_tab != TAB_INFO) {
      modal_active = (bool)!modal_active;
    }
    break;
  case 'q':
    return false;
  default:
    // Unrecognized key
    break;
  }

  return true;
}

static void draw_tab_line() {
  static constexpr char TABS[][16] = {
      {"General"},
      {"OC"},
      {"Thermals"},
      {"Info"},
  };
  static constexpr size_t TAB_COUNT = (sizeof(TABS) / sizeof(TABS[0]));

  int x_pos = 2;
  for (size_t tab_index = 1; tab_index <= TAB_COUNT; ++tab_index) {
    auto tab_name = TABS[tab_index - 1];
    char buffer[64];
    (void)snprintf(buffer, sizeof(buffer), " [%zu] %s ", tab_index, tab_name);
    if (tab_index == selected_tab) {
      attr_set(A_BOLD, PAIR_SELECTION, nullptr);
    }
    mvprintw(0, x_pos, "%s", buffer);
    x_pos += (int)strlen(buffer);
    attr_set(A_NORMAL, 0, nullptr);
  }
}

static void draw_title() {
  char buffer[16];
  (void)snprintf(buffer, sizeof(buffer), " %s ", TITLE);
  int x_pos = COLS - (int)strlen(buffer) - 2;
  attr_set(A_BOLD, PAIR_TITLE, nullptr);
  mvprintw(0, x_pos, "%s", buffer);
  attr_set(A_NORMAL, 0, nullptr);
}

static void draw_poll_controls() {
  char buffer[16];
  (void)snprintf(buffer, sizeof(buffer), " [-] %ums [+] ", poll_ms);
  int x_pos = COLS - (int)strlen(buffer) - 2;
  mvprintw(LINES - 1, x_pos, "%s", buffer);
}

static void draw_configuration_modal_toggle() {
  if (modal_active) {
    attr_set(A_BOLD, PAIR_SELECTION, nullptr);
  }
  mvprintw(LINES - 1, 4, "%s", "[c]onfigure plot");
  attr_set(A_NORMAL, 0, nullptr);
}

static void draw_frame() {
  mvhline(0, 1, ACS_HLINE, COLS - 2);
  mvhline(LINES - 1, 1, ACS_HLINE, COLS - 2);
  mvvline(1, 0, ACS_VLINE, LINES - 2);
  mvvline(1, COLS - 1, ACS_VLINE, LINES - 2);
  mvaddch(0, 0, ACS_ULCORNER);
  mvaddch(0, COLS - 1, ACS_URCORNER);
  mvaddch(LINES - 1, 0, ACS_LLCORNER);
  mvaddch(LINES - 1, COLS - 1, ACS_LRCORNER);
  draw_tab_line();
  draw_title();
  draw_poll_controls();
  if (selected_tab != TAB_INFO) {
    draw_configuration_modal_toggle();
  }
}

static void draw_general_tab(WINDOW *tab_page, const GpuState *state) {
  int y_pos = 1;
  int x_pos = 1;
  int cols = getmaxx(tab_page);

  wattr_set(tab_page, A_UNDERLINE, 0, nullptr);
  mvwprintw(tab_page, y_pos++, x_pos, "Core");
  wattr_set(tab_page, A_NORMAL, 0, nullptr);
  mvwprintw(tab_page, y_pos++, x_pos, "GPU Utilization: %u%%",
            state->gpu_util_percent);
  mvwprintw(tab_page, y_pos++, x_pos, "Clock Speed: %u Mhz",
            state->core_clock_mhz);
  y_pos++;

  wattr_set(tab_page, A_UNDERLINE, 0, nullptr);
  mvwprintw(tab_page, y_pos++, x_pos, "Memory");
  wattr_set(tab_page, A_NORMAL, 0, nullptr);

  mvwprintw(tab_page, y_pos++, x_pos, "Memory: %'u MiB/%'u MiB",
            state->used_vram_mib, state->usable_vram_mib);
  mvwprintw(tab_page, y_pos++, x_pos, "Memory Clock: %u Mhz",
            state->memory_clock_mhz);
  mvwprintw(tab_page, y_pos++, x_pos, "Controller Utilization: %u%%",
            state->mem_ctrl_util_percent);
  y_pos++;

  wattr_set(tab_page, A_UNDERLINE, 0, nullptr);
  mvwprintw(tab_page, y_pos++, x_pos, "Thermals");
  wattr_set(tab_page, A_NORMAL, 0, nullptr);
  mvwprintw(tab_page, y_pos++, x_pos, "Fan Speed: %u%% (%u RPM)",
            state->fan_speed_percentage, state->fan_speed_rpm);
  mvwprintw(tab_page, y_pos++, x_pos, "Temperature: %u°C",
            state->temperature_celsius);

  // Switch columns
  y_pos = 1;
  x_pos += cols / 2;

  mvwprintw(tab_page, y_pos++, x_pos, "P-State: %s", state->performance_state);
  mvwprintw(tab_page, y_pos++, x_pos, "Encoder: %u%%   Decoder: %u%%",
            state->encoder_util_percent, state->decoder_util_percent);
}

static void draw_oc_tab(WINDOW *tab_page, const GpuState *state,
                        const OCController *oc_controller) {
  int y_pos = 1;
  int x_pos = 1;
  int cols = getmaxx(tab_page);

  wattr_set(tab_page, A_UNDERLINE, 0, nullptr);
  mvwprintw(tab_page, y_pos++, x_pos, "Controls");
  wattr_set(tab_page, A_NORMAL, 0, nullptr);

  static constexpr int SLIDER_HEIGHT = 3;
  for (int i = 0; i < SLIDER_COUNT; ++i) {
    auto slider_window =
        derwin(tab_page, SLIDER_HEIGHT, ((cols - x_pos) / 2) - 1, y_pos, x_pos);
    assert(slider_window != nullptr);
    oc_controller_draw_slider(oc_controller, (Slider)i, slider_window);
    delwin(slider_window);
    y_pos += SLIDER_HEIGHT + 1;
  }

  y_pos = 1;
  x_pos += (cols / 2) + 1;

  mvwprintw(tab_page, y_pos++, x_pos, "[j/k] navigate [h/l] adjust");

  bool dirty = oc_controller_is_dirty(oc_controller);
  bool at_defaults = oc_controller_at_default_values(state);

  static constexpr char APPLY_LABEL[] = "[a]pply   ";
  static constexpr char DISCARD_LABEL[] = "[d]iscard   ";
  static constexpr char RESET_LABEL[] = "[r]eset";

  int discard_x = x_pos + (int)strlen(APPLY_LABEL);
  int reset_x = discard_x + (int)strlen(DISCARD_LABEL);

  if (dirty) {
    wattr_set(tab_page, A_BOLD, 0, nullptr);
  } else {
    wattr_set(tab_page, A_DIM, 0, nullptr);
  }
  mvwprintw(tab_page, y_pos, x_pos, "%s", APPLY_LABEL);
  mvwprintw(tab_page, y_pos, discard_x, "%s", DISCARD_LABEL);
  if (at_defaults) {
    wattr_set(tab_page, A_DIM, 0, nullptr);
  } else {
    wattr_set(tab_page, A_BOLD, 0, nullptr);
  }
  mvwprintw(tab_page, y_pos, reset_x, "%s", RESET_LABEL);
  wattr_set(tab_page, A_NORMAL, 0, nullptr);
  ++y_pos;

  if (!is_root) {
    wattr_set(tab_page, A_BOLD | A_ITALIC, PAIR_DANGER, nullptr);
    mvwprintw(tab_page, y_pos++, x_pos,
              "Not running as root -- controls disabled!");
    wattr_set(tab_page, A_NORMAL, 0, nullptr);
  } else {
    ++y_pos;
  }

  mvwprintw(tab_page, y_pos++, x_pos, "Power Draw: %u.%u W   Temp: %u°C",
            (state->power_draw_milliwatts / 100) / 10,
            (state->power_draw_milliwatts / 100) % 10,
            state->temperature_celsius);
  mvwprintw(tab_page, y_pos++, x_pos, "Fan Speed:  %u%% (%u RPM)",
            state->fan_speed_percentage, state->fan_speed_rpm);
  ++y_pos;

  wattr_set(tab_page, A_UNDERLINE, 0, nullptr);
  mvwprintw(tab_page, y_pos++, x_pos, "Throttle Reasons");
  wattr_set(tab_page, A_NORMAL, 0, nullptr);
  if (state->throttle_reason_count == 0) {
    wattr_set(tab_page, A_NORMAL, PAIR_SAFE, nullptr);
    mvwprintw(tab_page, y_pos++, x_pos, "None");
  }
  wattr_set(tab_page, A_NORMAL, PAIR_DANGER, nullptr);
  static constexpr unsigned int MAX_DISPLAYED_THROTTLE_REASONS = 4;
  unsigned int counter = 0;
  for (unsigned int i = 0; i < state->throttle_reason_count; ++i) {
    ++counter;
    if (counter > MAX_DISPLAYED_THROTTLE_REASONS) {
      mvwprintw(tab_page, y_pos, x_pos, "+ %u more",
                counter - MAX_DISPLAYED_THROTTLE_REASONS);
      continue;
    }
    mvwprintw(tab_page, y_pos++, x_pos, "%s", state->throttle_reasons[i]);
  }
  wattr_set(tab_page, A_NORMAL, 0, nullptr);
}

static void draw_thermals_tab(WINDOW *tab_page) {}

static void draw_info_tab(WINDOW *tab_page, const GpuState *state) {
  int y_pos = 1;
  int x_pos = 1;
  wattr_set(tab_page, A_UNDERLINE, PAIR_HEADING, nullptr);
  mvwprintw(tab_page, y_pos++, x_pos, "GPU Device Info");
  wattr_set(tab_page, A_NORMAL, 0, nullptr);
  mvwprintw(tab_page, y_pos++, x_pos, "Device:       %s", state->name);
  mvwprintw(tab_page, y_pos++, x_pos, "Architecture: %s", state->architecture);
  mvwprintw(tab_page, y_pos++, x_pos, "GPU Cores:    %u", state->num_gpu_cores);
  mvwprintw(tab_page, y_pos++, x_pos, "VRAM:         %'u MiB (%'u MiB usable)",
            state->total_vram_mib, state->usable_vram_mib);
  mvwprintw(tab_page, y_pos++, x_pos, "PCIe:         Gen %u x%u",
            state->pcie_max_link_generation, state->pcie_max_link_width);
  mvwprintw(tab_page, y_pos++, x_pos, "Fan Ctrls:    %u",
            state->num_fan_controllers);
  mvwprintw(tab_page, y_pos++, x_pos, "TDP:          %u W",
            state->tdp_milliwatts / 1000);
  mvwprintw(tab_page, y_pos++, x_pos, "VBIOS:        %s", state->vbios_version);
  ++y_pos;
  wattr_set(tab_page, A_UNDERLINE, PAIR_HEADING, nullptr);
  mvwprintw(tab_page, y_pos++, x_pos, "Drivers");
  wattr_set(tab_page, A_NORMAL, 0, nullptr);
  mvwprintw(tab_page, y_pos++, x_pos, "Driver Version: %s",
            state->driver_version);
  mvwprintw(tab_page, y_pos++, x_pos, "NVML Version:   %s",
            state->nvml_version);
  mvwprintw(tab_page, y_pos++, x_pos, "CUDA Version:   %s",
            state->cuda_version);
}

static void draw_content(WINDOW *tab_page, const GpuState *gpu_state,
                         const OCController *oc_controller,
                         PlotController *plot_controller) {
  static constexpr int PLOT_START_ROW = 14;
  switch (selected_tab) {
  case TAB_GENERAL:
    draw_general_tab(tab_page, gpu_state);
    auto plot_region = derwin(tab_page, getmaxy(tab_page) - PLOT_START_ROW,
                              getmaxx(tab_page), PLOT_START_ROW, 0);
    plot_controller_switch_profile(plot_controller, PLOT_PROFILE_GENERAL);
    plot_controller_draw(plot_controller, gpu_state, plot_region);
    delwin(plot_region);
    break;
  case TAB_OC:
    draw_oc_tab(tab_page, gpu_state, oc_controller);
    auto oc_plot_region = derwin(tab_page, getmaxy(tab_page) - PLOT_START_ROW,
                                 getmaxx(tab_page), PLOT_START_ROW, 0);
    plot_controller_switch_profile(plot_controller, PLOT_PROFILE_OC);
    plot_controller_draw(plot_controller, gpu_state, oc_plot_region);
    delwin(oc_plot_region);
    break;
  case TAB_THERMALS:
    plot_controller_switch_profile(plot_controller, PLOT_PROFILE_THERMALS);
    draw_thermals_tab(tab_page);
    break;
  case TAB_INFO:
    draw_info_tab(tab_page, gpu_state);
    break;
  default:
    return;
  }
}

void tui_init() {
  is_root = geteuid() == 0;

  if (setlocale(LC_ALL, "") == nullptr) {
    (void)fprintf(stderr, "warning: couldn't set locale\n");
  }

  initscr();
  init_colors();
  cbreak();
  noecho();
  keypad(stdscr, true);
  nodelay(stdscr, true);
  curs_set(0);
}

static unsigned long get_time_ms() {
  struct timespec time;
  clock_gettime(CLOCK_MONOTONIC, &time);
  return ((unsigned long)time.tv_sec * 1'000ULL) +
         ((unsigned long)time.tv_nsec / 1'000'000ULL);
}

static void draw(const GpuState *gpu_state, const OCController *oc_controller,
                 PlotController *plot_controller, ConfigModal *config_modal) {
  erase();

  draw_frame();
  auto tab_page = derwin(stdscr, LINES - 2, COLS - 2, 1, 1);
  draw_content(tab_page, gpu_state, oc_controller, plot_controller);

  wnoutrefresh(stdscr);

  if (modal_active) {
    int parent_x = getmaxx(tab_page);
    int parent_y = getmaxy(tab_page);

    int height = (parent_y * 2) / 3;
    int width = parent_x / 2;
    auto modal_window = derwin(tab_page, height, width, (parent_y - height) / 2,
                               (parent_x - width) / 2);
    assert(modal_window);
    config_modal_draw(config_modal, modal_window);

    wnoutrefresh(modal_window);
    doupdate();

    delwin(modal_window);
  } else {
    doupdate();
  }

  delwin(tab_page);
}

void tui_run(Gpu *gpu) {
  static constexpr struct timespec SLEEP_DURATION = {
      .tv_sec = 0, .tv_nsec = 15 * 1'000'000L};
  unsigned long current_time_ms = get_time_ms();
  unsigned long last_update_time_ms = current_time_ms;
  auto plot_controller = plot_controller_create();
  auto oc_controller = oc_controller_create(gpu_get_state(gpu));
  auto config_modal = plot_controller_get_config_modal(plot_controller);

  gpu_update_state(gpu);

  for (;;) {
    if (!enforce_minimum_size()) {
      return;
    }
    int current_key = ERR;
    while ((current_key = getch()) != ERR) {
      // Because the modal and the main window have disjoint sets of controls
      // we can just process one after an other, with special behavior for
      // quitting when the modal is active
      auto should_continue = handle_input(current_key);
      if (modal_active) {
        config_modal_handle_input(config_modal, current_key);
      } else if (selected_tab == TAB_OC && is_root) {
        oc_controller_handle_input(oc_controller, gpu, current_key);
      }
      if (!should_continue) {
        if (modal_active) {
          modal_active = false;
          continue;
        }
        plot_controller_destroy(plot_controller);
        oc_controller_destroy(oc_controller);
        return;
      }
    }
    current_time_ms = get_time_ms();
    unsigned long elapsed_time = current_time_ms - last_update_time_ms;
    auto gpu_state = gpu_get_state(gpu);
    if (elapsed_time >= poll_ms) {
      gpu_update_state(gpu);
      plot_controller_feed_data(plot_controller, gpu_state);
      last_update_time_ms = current_time_ms;
    }
    draw(gpu_state, oc_controller, plot_controller, config_modal);
    nanosleep(&SLEEP_DURATION, nullptr);
  }
}

void tui_shutdown() { endwin(); }
