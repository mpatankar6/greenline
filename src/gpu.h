#pragma once

static constexpr unsigned int MAX_THROTTLE_REASONS = 9;
static constexpr unsigned int MAX_THROTTLE_REASON_LEN = 16;

typedef struct {
  bool initialized;
  char driver_version[128];
  char nvml_version[128];
  char cuda_version[128];
  char name[128];
  char vbios_version[32];
  const char *architecture;
  unsigned int total_vram_mib;
  unsigned int usable_vram_mib;
  unsigned int used_vram_mib;
  unsigned int free_vram_mib;
  unsigned int encoder_util_percent;
  unsigned int decoder_util_percent;
  unsigned int gpu_util_percent;
  unsigned int mem_ctrl_util_percent;
  unsigned int core_clock_mhz;
  unsigned int memory_clock_mhz;
  unsigned int max_core_clock_mhz;
  unsigned int max_memory_clock_mhz;
  unsigned int temperature_celsius;
  unsigned int fan_speed_percentage;
  unsigned int fan_speed_rpm;
  unsigned int fan_target_percent;
  bool fan_auto;
  char performance_state[4]; // Ex: "P0", "P8", or "?"
  unsigned int num_fan_controllers;
  unsigned int num_gpu_cores;
  unsigned int pcie_max_link_generation;
  unsigned int pcie_max_link_width;
  unsigned int pcie_max_link_speed_mbps;
  unsigned int tdp_milliwatts;
  unsigned int power_limit_min_milliwatts;
  unsigned int power_limit_max_milliwatts;
  unsigned int power_draw_milliwatts;
  unsigned int power_limit_milliwatts;
  int gpc_clock_offset_mhz;
  int gpc_clock_offset_min_mhz;
  int gpc_clock_offset_max_mhz;
  int mem_clock_offset_mhz;
  int mem_clock_offset_min_mhz;
  int mem_clock_offset_max_mhz;
  char throttle_reasons[MAX_THROTTLE_REASONS][MAX_THROTTLE_REASON_LEN];
  unsigned int throttle_reason_count;
  unsigned int temp_shutdown_threshold_celsius;
  unsigned int temp_slowdown_threshold_celsius;
  unsigned int temp_gpu_max_threshold_celsius;
} GpuState;

typedef struct Gpu Gpu;

[[nodiscard]]
Gpu *gpu_init();

[[nodiscard]]
const GpuState *gpu_get_state(const Gpu *gpu);

void gpu_update_state(Gpu *gpu);

void gpu_set_power_limit(Gpu *gpu, unsigned int milliwatts);

void gpu_set_gpc_clock_offset(Gpu *gpu, int offset_mhz);

void gpu_set_mem_clock_offset(Gpu *gpu, int offset_mhz);

void gpu_set_fan_target(Gpu *gpu, unsigned int percent);

void gpu_set_fan_auto(Gpu *gpu);

void gpu_destroy(Gpu *gpu);
